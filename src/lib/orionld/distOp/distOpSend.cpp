/*
*
* Copyright 2022 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <stdio.h>                                               // snprintf
#include <string.h>                                              // strncmp, strncpy
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjBuilder.h"                                     // kjString, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/ApiVersion.h"                            // API_VERSION_NGSILD_V1
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/q/qRender.h"                                   // qRender
#include "orionld/distOp/distOpSend.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// HttpResponse -
//
typedef struct HttpResponse
{
  DistOp*     distOpP;
  CURL*       curlHandle;
  char*       buf;
  uint32_t    bufPos;
  uint32_t    bufLen;
  bool        mustBeFreed;
  char        preBuf[4 * 1024];  // Could be much smaller for POST/PATCH/PUT/DELETE (only cover errors, 1k would be more than enough)
} HttpResponse;


//
// Need a better way to create the URL.
// Working with chunks:
// 1. URL
// 2. "id"  (entity id or subscription id)
// 3. "attrName"   (for PATCH Attribute, for example
// 4. Linked list of uri params
//

// -----------------------------------------------------------------------------
//
// SList
//
typedef struct SList
{
  char*          sP;
  int            sLen;
  struct SList*  next;
} SList;



// -----------------------------------------------------------------------------
//
// ForwardUrlParts -
//
typedef struct ForwardUrlParts
{
  DistOp*  distOpP;      // From here we need "op", "entityId", "attrName", ...
  SList*   params;       // Linked list of strings (URI params)
  SList*   last;         // Last item in the 'params' list - used for linking in new items
} ForwardUrlParts;



// -----------------------------------------------------------------------------
//
// urlCompose -
//
char* urlCompose(ForwardUrlParts* urlPartsP, KjNode* endpointP)
{
  int totalLen = 0;

  //
  // Calculating the length of the resulting URL (PATH+PARAMS)
  //
  totalLen += strlen(endpointP->value.s);
  totalLen += strlen(orionldState.urlPath);

  // Adding the lengths of the uri params
  for (SList* paramP = urlPartsP->params; paramP != NULL; paramP = paramP->next)
  {
    totalLen += paramP->sLen + 1;  // +1: Either '?' or '&'
  }

  totalLen += 1;  // For the string delimiter ('\0')

  //
  // Allocating a buffer for the URL (now that we know the length needed)
  //
  char* url = kaAlloc(&orionldState.kalloc, totalLen);

  //
  // Filling in the URL, piece by piece
  //
  int nb = snprintf(url, totalLen, "%s%s", endpointP->value.s, orionldState.urlPath);

  if (urlPartsP->params == NULL)
    return url;

  url[nb++] = '?';
  for (SList* paramP = urlPartsP->params; paramP != NULL; paramP = paramP->next)
  {
    strcpy(&url[nb], paramP->sP);
    nb += paramP->sLen;

    if (paramP->next != NULL)
      url[nb++] = '&';
  }

  url[nb] = 0;
  return url;
}



// -----------------------------------------------------------------------------
//
// uriParamAdd -
//
static void uriParamAdd(ForwardUrlParts* urlPartsP, const char* key, const char* value, int totalLen)
{
  SList* sListP = (SList*) kaAlloc(&orionldState.kalloc, sizeof(SList));

  if (value == NULL)
  {
    // If 'value' == NULL, then 'key' is complete (e.g.
    sListP->sP   = (char*) key;
    sListP->sLen = (totalLen == -1)? strlen(key) : totalLen;
  }
  else
  {
    int sLen     = strlen(key) + 1 + strlen(value) + 1;
    sListP->sP   = kaAlloc(&orionldState.kalloc, sLen);

    sListP->sLen = snprintf(sListP->sP, sLen, "%s=%s", key, value);
  }

  LM_T(LmtDistOpRequestParams, ("DistOp Request URL Param: %s", sListP->sP));

  sListP->next = NULL;

  if (urlPartsP->params == NULL)
    urlPartsP->params = sListP;
  else
    urlPartsP->last->next = sListP;

  urlPartsP->last = sListP;
}



// -----------------------------------------------------------------------------
//
// responseSave -
//
// NOTE
// According to https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html:
// - chunk:    points to the delivered data
// - members:  the size of that data
// - size:     is always 1.
//
static int responseSave(void* chunk, size_t size, size_t members, void* userP)
{
  char*            chunkP        = (char*) chunk;
  size_t           chunkLen      = members;
  HttpResponse*    httpResponseP = (HttpResponse*) userP;

  LM_T(LmtDistOpResponseBuf, ("Got %d*%d (%d) bytes of response from reg %s", size, members, size*members, httpResponseP->distOpP->regP->regId));

  //
  // Allocate room for the new piece
  //
  // 1. Preallocated buffer - If total size (httpResponseP->bufPos + chunkLen) < sizeof(httpResponseP->preBuf)
  // 2. kalloc buffer       - If total size <= 20k
  // 3. malloc              - AND set httpResponseP->mustBeFreed
  //
  uint32_t newSize = httpResponseP->bufPos + chunkLen;

  if (newSize < httpResponseP->bufLen)
  {
    LM_T(LmtDistOpResponseBuf, ("Copying %d bytes to httpResponseP->buf", chunkLen));
    strncpy(&httpResponseP->buf[httpResponseP->bufPos], chunkP, chunkLen);
    LM_T(LmtDistOpResponseBuf, ("httpResponseP->buf: '%s'", httpResponseP->buf));
  }
  else if (newSize >= httpResponseP->bufLen)
  {
    char* oldBuf = httpResponseP->buf;

    if (newSize < 20480)  // < 20k : use kalloc
    {
      httpResponseP->buf = kaAlloc(&orionldState.kalloc, newSize + 1024);
      if (httpResponseP->buf == NULL)
        LM_RE(1, ("Out of memory (kaAlloc failed allocating %d bytes for response buffer)", newSize + 1024));

      LM_T(LmtDistOpResponseBuf, ("Copying %d bytes to httpResponseP->buf", chunkLen));
      snprintf(httpResponseP->buf, newSize + 1023, "%s%s", oldBuf, chunkP);
      LM_T(LmtDistOpResponseBuf, ("httpResponseP->buf: '%s'", httpResponseP->buf));
    }
    else
    {
      httpResponseP->buf = (char*) malloc(newSize + 1024);
      if (httpResponseP->buf == NULL)
        LM_RE(1, ("Out of memory (allocating %d bytes for response buffer)", newSize + 1024));

      LM_T(LmtDistOpResponseBuf, ("Copying %d bytes to httpResponseP->buf", chunkLen));
      snprintf(httpResponseP->buf, newSize + 1023, "%s%s", oldBuf, chunkP);
      LM_T(LmtDistOpResponseBuf, ("httpResponseP->buf: '%s'", httpResponseP->buf));

      if (httpResponseP->mustBeFreed == true)
        free(oldBuf);
      httpResponseP->mustBeFreed = true;
    }

    httpResponseP->bufLen = newSize + 1024;
  }

  httpResponseP->bufPos = newSize;

  httpResponseP->distOpP->rawResponse = httpResponseP->buf;  // Cause ... it might have changed
  LM_T(LmtDistOpResponseBuf, ("%s: rawResponse now points to httpResponseP->buf (%p)", httpResponseP->distOpP->regP->regId, httpResponseP->buf));
  LM_T(LmtDistOpResponseBuf, ("httpResponseP->distOpP->rawResponse at %p: %s", httpResponseP->distOpP->rawResponse, httpResponseP->distOpP->rawResponse));
  return chunkLen;
}



// -----------------------------------------------------------------------------
//
// responseHeaderDebug -
//
static size_t responseHeaderDebug(char* buffer, size_t size, size_t nitems, void* userdata)
{
  LM_T(LmtDistOpResponseHeaders, ("Response Header: %s", buffer));
  return nitems;
}



// -----------------------------------------------------------------------------
//
// subAttrsCompact -
//
void subAttrsCompact(KjNode* requestBody, OrionldContext* fwdContextP)
{
  for (KjNode* subAttrP = requestBody->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
  {
    if (strcmp(subAttrP->name, "type")        == 0)   continue;
    if (strcmp(subAttrP->name, "value")       == 0)   continue;
    if (strcmp(subAttrP->name, "object")      == 0)   continue;
    if (strcmp(subAttrP->name, "languageMap") == 0)   continue;
    if (strcmp(subAttrP->name, "datasetId")   == 0)   continue;

    subAttrP->name = orionldContextItemAliasLookup(fwdContextP, subAttrP->name, NULL, NULL);
  }
}



// -----------------------------------------------------------------------------
//
// bodyCompact -
//
void bodyCompact(DistOpType operation, KjNode* requestBody, OrionldContext* fwdContextP)
{
  if ((operation == DoUpdateAttrs) || (operation == DoReplaceAttr))
    subAttrsCompact(requestBody, fwdContextP);
  else if ((operation == DoCreateEntity) || (operation == DoMergeEntity))
  {
    for (KjNode* attrP = requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      if (strcmp(attrP->name, "id")       == 0)  continue;
      if (strcmp(attrP->name, "scope")    == 0)  continue;
      if (strcmp(attrP->name, "location") == 0)  continue;

      if (strcmp(attrP->name, "type")  == 0)
      {
        attrP->value.s = orionldContextItemAliasLookup(fwdContextP, attrP->value.s, NULL, NULL);
        continue;
      }

      attrP->name = orionldContextItemAliasLookup(fwdContextP, attrP->name, NULL, NULL);

      subAttrsCompact(attrP, fwdContextP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// distOpSend -
//
bool distOpSend(DistOp* distOpP, const char* dateHeader, const char* xForwardedForHeader, const char* viaHeader, bool local, const char* entityIds)
{
  //
  // Figure out the @context to use for the forwarded request
  // 1. Default value is, of course, the Core Context
  // 2. Next up is the @context used in the original request, the one provoking the forwarded request
  // 3. The registration might have its own @context (from "jsonldContext" in the cSourceInfo array)
  //
  // Actually, point 1 is not necessary as orionldState.contextP (@context used in the original request) will point to the Core Context
  // if no @context was used in the original request
  //
  //
  OrionldContext* fwdContextP = (distOpP->regP->contextP != NULL)? distOpP->regP->contextP : orionldState.contextP;

  //
  // For now we only support http and https for forwarded requests. libcurl is used for both of them
  // So, a single function with an "if" or two will do - copy from orionld/notifications/httpsNotify.cpp
  //
  // * CURL Handles:  create handle (also multi handle if it does not exist)
  // * URL:           distOpP->url
  // * BODY:          kjFastRender(distOpP->requestBody) + CURLOPT_POSTFIELDS (POST? hmmm ...)
  // * HTTP Headers:  Std headers + loop over distOpP->regP["csourceInfo"], use curl_slist_append
  // * CURL Options:  set a number oftions dfor the handle
  // * SEND:          Actually, just add the "easy handler" to the "multi handler" of orionldState.curlDoMultiP
  //                  Once all requests are added - send by invoking 'curl_multi_perform(orionldState.curlDoMultiP, ...)'
  //

  //
  // CURL Handles
  //
  if (orionldState.curlDoMultiP == NULL)
  {
    orionldState.curlDoMultiP = curl_multi_init();
    if (orionldState.curlDoMultiP == NULL)
    {
      LM_E(("Internal Error: curl_multi_init failed"));
      return false;
    }
  }

  distOpP->curlHandle = curl_easy_init();
  LM_T(LmtLeak, ("Got a curl handle at %p", distOpP->curlHandle));
  if (distOpP->curlHandle == NULL)
  {
    curl_multi_cleanup(orionldState.curlDoMultiP);
    orionldState.curlDoMultiP = NULL;
    LM_E(("Internal Error: curl_easy_init failed"));
    return false;
  }

  //
  // The Content-Type of the distributed request is the same as the original request, unless "jsonldContext"
  // is part of "contextSourceInfo" of the registration (in which case Content-Type is always JSON).
  //
  MimeType contentType = orionldState.in.contentType;

  //
  // URL
  //
  KjNode*         endpointP = kjLookup(distOpP->regP->regTree, "endpoint");
  ForwardUrlParts urlParts  = { distOpP, NULL, NULL };

  //
  // Add URI Params
  //
  LM_T(LmtDistOpRequestParams, ("%s: ---- URL Parameters for %s ------------------------", distOpP->regP->regId, distOpP->id));
  if (orionldState.verb == HTTP_GET)
  {
    if (distOpP->attrsParam != NULL)
      uriParamAdd(&urlParts, distOpP->attrsParam, NULL, distOpP->attrsParamLen);

    //
    // Forwarded requests are ALWAYS sent with options=sysAttrs  (normalized is already default - no need to add that)
    // They MUST be sent with NORMALIZED and SYSATTRS, as without that, there's no way to pick attributes in case we have clashes
    //
    // There is one exception though - when only the entity ids are wanted as output
    //
    if (distOpP->onlyIds == true)
      uriParamAdd(&urlParts, "onlyIds=true", NULL, 12);
    else
      uriParamAdd(&urlParts, "options=sysAttrs", NULL, 16);

    if (distOpP->operation == DoQueryEntity)
    {
      if (entityIds != NULL)
        uriParamAdd(&urlParts, "id", entityIds, -1);
      else if (distOpP->entityId != NULL)
        uriParamAdd(&urlParts, "id", distOpP->entityId, -1);
    }

    if (local == true)
      uriParamAdd(&urlParts, "local=true", NULL, 10);

    if (distOpP->qNode != NULL)
    {
      char buf[256];
      qRender(distOpP->qNode, API_VERSION_NGSILD_V1, buf, sizeof(buf), NULL);
      LM_T(LmtDistOpRequestParams, ("DistOp %s has a Q: %s", distOpP->regP->regId, buf));
      if (orionldState.uriParams.q != NULL)
        LM_T(LmtDistOpRequestParams, ("The initial request also has a 'q'"));
    }

    //
    // If we know the Entity Type, we pass that piece of information as well
    //
    if (distOpP->entityType != NULL)
    {
      char* typeAlias = orionldContextItemAliasLookup(fwdContextP, distOpP->entityType, NULL, NULL);
      uriParamAdd(&urlParts, "type", typeAlias, -1);
    }
  }

  if (orionldState.uriParams.lang != NULL)
    uriParamAdd(&urlParts, "lang", orionldState.uriParams.lang, -1);

  if ((distOpP->operation == DoAppendAttrs) && (orionldState.uriParamOptions.noOverwrite == true))
    uriParamAdd(&urlParts, "options", "noOverwrite", 11);

  if (orionldState.uriParams.qCopy != NULL)
  {
    uriParamAdd(&urlParts, "q", orionldState.uriParams.qCopy, -1);
    LM_T(LmtDistOpRequestHeaders, ("%s: orionldState.uriParams.q: '%s'", distOpP->regP->regId, orionldState.uriParams.qCopy));
  }

  LM_T(LmtDistOpRequestParams, ("%s: ---- End of URL Parameters -----------------", distOpP->regP->regId));

  //
  // Compose the entire URL and pass it to CURL
  //
  char* url = urlCompose(&urlParts, endpointP);
  curl_easy_setopt(distOpP->curlHandle, CURLOPT_URL, url);

  bool https = (strncmp(endpointP->value.s, "https", 5) == 0);


  //
  // HTTP Headers
  // - Content-Length
  // - Content-Type
  // - User-Agent
  // - Date
  // - X-Forwarded-For
  // - Those in csourceInfo (if Content-Type is present, it is ignored (for now))
  // - NGSILD-Tenant (part of registration, but can also be in csourceInfo?)
  // - Link - can be in csourceInfo (named jsonldContext)
  //
  KjNode*             csourceInfoP = kjLookup(distOpP->regP->regTree, "contextSourceInfo");
  KjNode*             tenantP      = kjLookup(distOpP->regP->regTree, "tenant");
  struct curl_slist*  headers      = NULL;

  // Date
  headers = curl_slist_append(headers, dateHeader);

  // X-Forwarded-For
  headers = curl_slist_append(headers, xForwardedForHeader);

  // Via
  headers = curl_slist_append(headers, viaHeader);

  // Custom headers from Registration::contextSourceInfo
  char* infoTenant    = NULL;
  char* accept        = NULL;
  char* jsonldContext = NULL;

  if (csourceInfoP != NULL)
  {
    for (KjNode* regHeaderP = csourceInfoP->value.firstChildP; regHeaderP != NULL; regHeaderP = regHeaderP->next)
    {
      KjNode* keyP   = kjLookup(regHeaderP, "key");
      KjNode* valueP = kjLookup(regHeaderP, "value");

      if ((keyP == NULL) || (valueP == NULL))                     { LM_W(("Missing key or value in Registration::contextSourceInfo")); continue; }
      if ((keyP->type != KjString) || (valueP->type != KjString)) { LM_W(("key or value in Registration::contextSourceInfo is non-string")); continue; }
      if (strcasecmp(keyP->value.s, "Content-Type") == 0)         { LM_W(("Content-Type is part of the Registration::contextSourceInfo headers, however, that is not implemented yet, sorry")); continue; }

      //
      // Whatever the key is, if the value is "urn:ngsi-ld:request", then the value is to be taken from the HTTP headers of the original request.
      //
      if (strcmp(valueP->value.s, "urn:ngsi-ld:request") == 0)
      {
        //
        // Forward the header "as is" - look it up in the list of incoming headers (orionldState.in.httpHeaders)
        //
        if (orionldState.in.httpHeaders != NULL)
        {
          KjNode* kvP = kjLookup(orionldState.in.httpHeaders, keyP->value.s);

          if ((kvP != NULL) && (kvP->type == KjString))
          {
            int    size   = strlen(kvP->name) + 2 + strlen(kvP->value.s) + 1;
            char*  header = kaAlloc(&orionldState.kalloc, size + 1);

            snprintf(header, size, "%s: %s", kvP->name, kvP->value.s);
            headers = curl_slist_append(headers, header);
          }
        }

        continue;
      }

      if (strcasecmp(keyP->value.s, "NGSILD-Tenant") == 0)
      {
        infoTenant = valueP->value.s;
        continue;
      }

      if (strcasecmp(keyP->value.s, "jsonldContext") == 0)
      {
        jsonldContext = valueP->value.s;
        contentType = MT_JSON;
        continue;
      }
      if (strcasecmp(keyP->value.s, "Accept") == 0)
        accept = valueP->value.s;

      // None of the above, adding the header
      char  header[256];  // Assuming 256 is enough
      snprintf(header, sizeof(header), "%s: %s", keyP->value.s, valueP->value.s);
      headers = curl_slist_append(headers, header);
    }
  }

  //
  // Accept - application/json (unless present in Registration::contextSourceInfo
  //
  if (accept == NULL)
  {
    const char* accept = "Accept: application/json";
    headers = curl_slist_append(headers, accept);
  }

  // FIXME: This field "acceptJsonld" should be done at registration creation/patching time and not here!
  distOpP->regP->acceptJsonld = (accept != NULL) && (strcmp(accept, "application/ld+json") == 0);

  //
  // Link header must be added if "Content-Type" is "application/json" in original request
  // OR: if jsonldContext is present in "contextSourceInfo"
  //
  if (orionldState.verb == HTTP_GET)
    contentType = MT_JSON;

  if (contentType == MT_JSON)
  {
    // Link header to be added if present in Registration::contextSourceInfo OR if not Core Context
    if (distOpP->regP->contextP != NULL)
      jsonldContext = distOpP->regP->contextP->url;
    else if ((jsonldContext == NULL) && (orionldState.contextP != orionldCoreContextP))
      jsonldContext = orionldState.contextP->url;

    if (jsonldContext != NULL)
    {
      char linkHeader[512];
      snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"", jsonldContext);
      headers = curl_slist_append(headers, linkHeader);
    }
  }

  // Tenant
  char* tenant = NULL;
  if (tenantP != NULL)
    tenant = tenantP->value.s;
  else if (infoTenant != NULL)
    tenant = infoTenant;
  else if (orionldState.tenantP != &tenant0)
    tenant = orionldState.tenantP->tenant;

  if (tenant != NULL)
  {
    char tenantHeader[80];
    snprintf(tenantHeader, sizeof(tenantHeader), "NGSILD-Tenant: %s", tenant);
    headers = curl_slist_append(headers, tenantHeader);
  }


  // User-Agent
  headers = curl_slist_append(headers, userAgentHeaderNoLF);  // userAgentHeader is initialized in orionldServiceInit()
  // headers = curl_slist_append(headers, "User-Agent: orionld/xxx");  // This works


#if 0
  struct curl_slist* sP = headers;
  while (sP != NULL)
  {
    LM_T(LmtDistOpRequest, ("FWD: Added header '%s'", sP->data));
    sP = sP->next;
  }
#endif

  //
  // BODY
  //
  int   approxContentLen = 0;
  char* payloadBody      = NULL;

  if (distOpP->requestBody != NULL)
  {
    // The payload body of attribute requests were transformed into an "Entity" for regMatch purposes.
    // Before forwarding, that trandformation needs to go back
    //
    if (distOpP->operation == DoReplaceAttr)
      distOpP->requestBody = distOpP->requestBody->value.firstChildP;

    //
    // Here we know the @context and we can compact the payload body
    //
    bodyCompact(distOpP->operation, distOpP->requestBody, fwdContextP);

    //
    // Content-Type (for now we maintain the original Content-Type in the forwarded request
    // First we remove it from the "headers" curl_slist, so that we can formulate it ourselves
    //
    const char* contentTypeString = (contentType == MT_JSON)? "Content-Type: application/json" : "Content-Type: application/ld+json";
    headers = curl_slist_append(headers, contentTypeString);

    if (contentType == MT_JSONLD)
    {
      //
      // Add the context to the body if not there already
      // - It can't be there, can it?
      // - If it is, it might not be the correct one!
      //
      // ALSO:
      //   This doesn't work for arrays, only for objects!
      //
      KjNode* contextP = kjLookup(distOpP->requestBody, "@context");

      if (contextP != NULL)
        kjChildRemove(distOpP->requestBody, contextP);

      if (distOpP->regP->contextP == NULL)
        contextP = kjString(orionldState.kjsonP, "@context", orionldCoreContextP->url);
      else
        contextP = kjString(orionldState.kjsonP, "@context", distOpP->regP->contextP->url);

      kjChildAdd(distOpP->requestBody, contextP);
    }

    approxContentLen = kjFastRenderSize(distOpP->requestBody);
    payloadBody      = kaAlloc(&orionldState.kalloc, approxContentLen);

    kjFastRender(distOpP->requestBody, payloadBody);

    //
    // Content-Length
    //
    int  contentLen = strlen(payloadBody);
    char contentLenHeader[64];
    snprintf(contentLenHeader, sizeof(contentLenHeader), "Content-Length: %d", contentLen);
    headers = curl_slist_append(headers, contentLenHeader);

    // BODY
    curl_easy_setopt(distOpP->curlHandle, CURLOPT_POSTFIELDS, payloadBody);
  }


  //
  // Need to save the pointer to the curl headers in order to free it afterwards
  //
  distOpP->curlHeaders = headers;

  //
  // CURL Options
  //
  curl_easy_setopt(distOpP->curlHandle, CURLOPT_CUSTOMREQUEST, orionldState.verbString);
  curl_easy_setopt(distOpP->curlHandle, CURLOPT_TIMEOUT_MS, 5000);                     // Timeout - hard-coded to 5 seconds for now ...
  // curl_easy_setopt(distOpP->curlHandle, CURLOPT_FAILONERROR, true);                    // Fail On Error - to detect 404 etc.
  curl_easy_setopt(distOpP->curlHandle, CURLOPT_FOLLOWLOCATION, 1L);                   // Follow redirections

  // Debugging Incoming HTTP Headers?
  if (lmTraceIsSet(LmtDistOpResponseHeaders) == true)
    curl_easy_setopt(distOpP->curlHandle, CURLOPT_HEADERFUNCTION, responseHeaderDebug);   // Callback for headers


  //
  // Set the callback function (responseSave) for reading the response
  //
  HttpResponse* httpResponseP = (HttpResponse*) kaAlloc(&orionldState.kalloc, sizeof(HttpResponse));

  httpResponseP->distOpP      = distOpP;
  httpResponseP->curlHandle   = distOpP->curlHandle;
  httpResponseP->buf          = httpResponseP->preBuf;
  httpResponseP->bufPos       = 0;
  httpResponseP->bufLen       = sizeof(httpResponseP->preBuf);
  httpResponseP->mustBeFreed  = false;

  httpResponseP->distOpP->rawResponse = httpResponseP->buf;

  curl_easy_setopt(distOpP->curlHandle, CURLOPT_WRITEFUNCTION, responseSave);          // Callback for reading the response body
  curl_easy_setopt(distOpP->curlHandle, CURLOPT_WRITEDATA, (void*) httpResponseP);     // User data for responseSave

  if (https)
  {
    // SSL options
    curl_easy_setopt(distOpP->curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);                 // ignore self-signed certificates for SSL end-points
    curl_easy_setopt(distOpP->curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);                 // DO NOT verify the certificate's name against host
  }

  curl_easy_setopt(distOpP->curlHandle, CURLOPT_HTTPHEADER, headers);

  //
  // SEND (sort of - enqueue the request)
  //
  LM_T(LmtDistOpRequest, ("%s: distributed request '%s' %s %s '%s'", distOpP->regP->regId, distOpP->id, orionldState.verbString, url, (payloadBody == NULL)? "no body" : payloadBody));
  curl_multi_add_handle(orionldState.curlDoMultiP, distOpP->curlHandle);

  return 0;
}
