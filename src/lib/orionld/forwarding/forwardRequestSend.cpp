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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/forwardRequestSend.h"               // Own interface



// -----------------------------------------------------------------------------
//
// HttpResponse -
//
typedef struct HttpResponse
{
  ForwardPending*  fwdPendingP;
  CURL*            curlHandle;
  char*            buf;
  uint32_t         bufPos;
  uint32_t         bufLen;
  bool             mustBeFreed;
  // Linked List of Response headers
  char             preBuf[4 * 1024];  // Could be much smaller for POST/PATCH/PUT/DELETE (only cover errors, 1k would be more than enough)
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
  ForwardPending*  fwdPendingP;  // From here we need "op", "entityId", "attrName", ...
  SList*           params;       // Linked list of strings (URI params)
  SList*           last;         // Last item in the 'params' list - used for linking in new items
} ForwardUrlParts;



// -----------------------------------------------------------------------------
//
// urlPath -
//
int urlPath(char* url, int urlLen, ForwardUrlParts* urlPartsP, KjNode* endpointP)
{
  int nb = 0;

  if (urlPartsP->fwdPendingP->operation == FwdCreateEntity)
    nb = snprintf(url, urlLen, "%s/ngsi-ld/v1/entities", endpointP->value.s);
  else if (urlPartsP->fwdPendingP->operation == FwdRetrieveEntity)
    nb = snprintf(url, urlLen, "%s/ngsi-ld/v1/entities/%s", endpointP->value.s, urlPartsP->fwdPendingP->entityId);
  else
    LM_X(1, ("Forwarding is not implemented for '%s' requests", fwdOperations[urlPartsP->fwdPendingP->operation]));

  return nb;
}



// -----------------------------------------------------------------------------
//
// urlCompose -
//
char* urlCompose(ForwardUrlParts* urlPartsP, KjNode* endpointP)
{
  int totalLen = 0;

  //
  // Calculating the length of the resulting URL
  //
  totalLen += strlen(endpointP->value.s);
  totalLen += fwdOperationUrlLen[urlPartsP->fwdPendingP->operation];

  if (urlPartsP->fwdPendingP->entityId != NULL)
    totalLen += strlen(urlPartsP->fwdPendingP->entityId) + 1;  // +1: the '/' after "/entities" and before the entityId
  if (urlPartsP->fwdPendingP->attrName != NULL)
    totalLen += strlen(urlPartsP->fwdPendingP->attrName) + 1;  // +1: the '/' after "/attrs" and before the attrName

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
  int nb = urlPath(url, totalLen, urlPartsP, endpointP);

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
void uriParamAdd(ForwardUrlParts* urlPartsP, const char* key, const char* value, int totalLen)
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
static int responseSave(void* chunk, size_t size, size_t members, void* userP)
{
  char*            chunkP        = (char*) chunk;
  size_t           chunkLen      = size * members;
  HttpResponse*    httpResponseP = (HttpResponse*) userP;

  //
  // Allocate room for the new piece
  //
  // 1. Preallocated buffer - If total size (httpResponseP->bufPos + chunkLen) < sizeof(httpResponseP->preBuf)
  // 2. kalloc buffer       - If total size <= 20k
  // 3. malloc              - AND set httpResponseP->mustBeFreed
  //
  uint32_t newSize = httpResponseP->bufPos + chunkLen;

  if (newSize < httpResponseP->bufLen)
    strncpy(&httpResponseP->buf[httpResponseP->bufPos], chunkP, chunkLen);
  else if (newSize >= httpResponseP->bufLen)
  {
    char* oldBuf = httpResponseP->buf;

    if (newSize < 20480)  // < 20k : use kalloc
    {
      httpResponseP->buf = kaAlloc(&orionldState.kalloc, newSize + 1024);
      if (httpResponseP->buf == NULL)
        LM_RE(1, ("Out of memory (kaAlloc failed allocating %d bytes for response buffer)", newSize + 1024));

      snprintf(httpResponseP->buf, newSize + 1023, "%s%s", oldBuf, chunkP);
    }
    else  // > 20k: use malloc
    {
      httpResponseP->buf = (char*) malloc(newSize + 1024);
      if (httpResponseP->buf == NULL)
        LM_RE(1, ("Out of memory (allocating %d bytes for response buffer)", newSize + 1024));

      snprintf(httpResponseP->buf, newSize + 1023, "%s%s", oldBuf, chunkP);

      if (httpResponseP->mustBeFreed == true)
        free(oldBuf);
      httpResponseP->mustBeFreed = true;
    }

    httpResponseP->fwdPendingP->rawResponse = httpResponseP->buf;  // Cause ... it has changed
    httpResponseP->bufLen = newSize + 1024;
  }

  httpResponseP->bufPos = newSize;

  return chunkLen;
}



// -----------------------------------------------------------------------------
//
// attrsParam -
//
void attrsParam(OrionldContext* contextP, ForwardUrlParts* urlPartsP, StringArray* attrList)
{
  if (contextP == NULL)
    contextP = orionldCoreContextP;

  //
  // The attributes are in longnames but ... should probably compact them.
  // A registration can have its own @context, in cSourceInfo - for now, we use the @context of the original request.
  // The attrList is always cloned, so, no problem modifying it.
  //
  int attrsLen = 0;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    LM(("Compacting attr '%s' for forwarding, using context '%s'", attrList->array[ix], contextP->url));
    attrList->array[ix]  = orionldContextItemAliasLookup(contextP, attrList->array[ix], NULL, NULL);
    attrsLen            += strlen(attrList->array[ix]) + 1;
    LM(("Compacted attr: '%s'", attrList->array[ix]));
  }

  // Make room for "attrs=" and the string-end zero
  attrsLen += 7;

  char* attrs = kaAlloc(&orionldState.kalloc, attrsLen);

  strcpy(attrs, "attrs=");

  int   pos = 6;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    int len = strlen(attrList->array[ix]);
    strcpy(&attrs[pos], attrList->array[ix]);

    // Add comma unless it's the last attr (in which case we add a zero, just in case
    pos += len;

    if (ix != attrList->items - 1)  // Not the last attr
    {
      attrs[pos] = ',';
      pos += 1;
    }
    else
      attrs[pos] = 0;
  }

  uriParamAdd(urlPartsP, attrs, NULL, pos);
}



// -----------------------------------------------------------------------------
//
// forwardRequestSend -
//
bool forwardRequestSend(ForwardPending* fwdPendingP, const char* dateHeader)
{
  //
  // Figure out the @context to use for the forwarded request
  // 1. Default value is, of course, the Core Context
  // 2. Next up is the @context used in the original request, the one provoking the forwarded request
  // 3. The registration might have its own @context (from "jsonldContext" in the cSourceInfo array
  //
  // Actually, point 1 is not necessary as orionldState.contextP (@context used in the original request) will point to the Core Context
  // if no @context was used in the original request
  //
  //
  OrionldContext* fwdContextP = (fwdPendingP->regP->contextP != NULL)? fwdPendingP->regP->contextP : orionldState.contextP;

  //
  // For now we only support http and https for forwarded requests. libcurl is used for both of them
  // So, a single function with an "if" or two will do - copy from orionld/notifications/httpsNotify.cpp
  //
  // * CURL Handles:  create handle (also multi handle if it does not exist)
  // * URL:           fwdPendingP->url
  // * BODY:          kjFastRender(fwdPendingP->body) + CURLOPT_POSTFIELDS (POST? hmmm ...)
  // * HTTP Headers:  Std headers + loop over fwdPendingP->regP["csourceInfo"], use curl_slist_append
  // * CURL Options:  set a number oftions dfor the handle
  // * SEND:          Actually, just add the "easy handler" to the "multi handler" of orionldState.curlFwdMultiP
  //                  Once all requests are added - send by invoking 'curl_multi_perform(orionldState.curlFwdMultiP, ...)'
  //

  //
  // CURL Handles
  //
  if (orionldState.curlFwdMultiP == NULL)
  {
    orionldState.curlFwdMultiP = curl_multi_init();
    if (orionldState.curlFwdMultiP == NULL)
    {
      LM_E(("Internal Error: curl_multi_init failed"));
      return false;
    }
  }

  fwdPendingP->curlHandle = curl_easy_init();
  if (fwdPendingP->curlHandle == NULL)
  {
    LM_E(("Internal Error: curl_easy_init failed"));
    return false;
  }

  //
  // URL
  //
  KjNode*         endpointP = kjLookup(fwdPendingP->regP->regTree, "endpoint");
  ForwardUrlParts urlParts  = { fwdPendingP, NULL, NULL };

  //
  // Add URI Params
  //
  if (orionldState.verb == GET)
  {
    if ((fwdPendingP->attrList != NULL) && (fwdPendingP->attrList->items > 0))
      attrsParam(fwdContextP, &urlParts, fwdPendingP->attrList);

    //
    // Forwarded requests are ALWAYS sent with options=sysAttrs  (normalized is already default - no need to add that)
    // They MUST be sent with NOEMALIZED and SYSATTRS, as with out that, there's no way to pick attributes in case we have clashes
    //
    uriParamAdd(&urlParts, "options=sysAttrs", NULL, 16);
  }

  if (orionldState.uriParams.lang != NULL)
    uriParamAdd(&urlParts, "lang", orionldState.uriParams.lang, -1);

  //
  // Compose the entire URL and pass it to CURL
  //
  char* url = urlCompose(&urlParts, endpointP);
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_URL, url);

  bool https = (strncmp(endpointP->value.s, "https", 5) == 0);


  //
  // HTTP Headers
  // - Content-Length
  // - Content-Type
  // - User-Agent
  // - Date
  // - Those in csourceInfo (if Content-Type is present, it is ignored (for now))
  // - NGSILD-Tenant (part of registration, but can also be in csourceInfo?)
  // - Link - can be in csourceInfo (named jsonldContext)
  //
  KjNode*             csourceInfoP = kjLookup(fwdPendingP->regP->regTree, "contextSourceInfo");
  KjNode*             tenantP      = kjLookup(fwdPendingP->regP->regTree, "tenant");
  struct curl_slist*  headers      = NULL;

  // Date
  headers = curl_slist_append(headers, dateHeader);

  // Host
  headers = curl_slist_append(headers, hostHeader);

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

      if (strcasecmp(keyP->value.s, "NGSILD-Tenant") == 0)
      {
        infoTenant = valueP->value.s;
        continue;
      }

      if (strcasecmp(keyP->value.s, "jsonldContext") == 0)
      {
        jsonldContext = valueP->value.s;
        LM(("************* jsonldContext: %s", jsonldContext));
        continue;
      }

      if (strcasecmp(keyP->value.s, "Accept") == 0)
        accept = valueP->value.s;

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

  // FIXME: This field "acceptJsonld" should be done at creation/patching time and not here!
  fwdPendingP->regP->acceptJsonld = (accept != NULL) && (strcmp(accept, "application/ld+json") == 0);

  //
  // Link header must be added if "Content-Type" is "application/json" in original request
  //
  if (orionldState.verb == GET)
    orionldState.in.contentType = JSON;

  if (orionldState.in.contentType == JSON)
  {
    // Link header to be added if present in Registration::contextSourceInfo OR if not Core Context
    if ((jsonldContext == NULL) && (orionldState.contextP != orionldCoreContextP))
      jsonldContext = orionldState.contextP->url;

    if (jsonldContext != NULL)
    {
      char linkHeader[512];
      snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"", jsonldContext);
      headers = curl_slist_append(headers, linkHeader);
      LM(("Added Link Header '%s'", orionldState.contextP->url));
    }
    else
      LM(("No Link Header added - Core Context (jsonldContext == NULL)"));
  }
  else
    LM(("No Link Header added - Core Context (orionldState.in.contentType != JSON  (%d))", orionldState.in.contentType));

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
    char tenantHeader[64];
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
    LM(("FWD: Added header '%s'", sP->data));
    sP = sP->next;
  }
#endif

  //
  // BODY
  //
  int   approxContentLen = 0;
  char* payloadBody      = NULL;

  if (fwdPendingP->body != NULL)
  {
    //
    // Content-Type (for now we maintain the original Content-Type in the forwarded request
    // First we remove it from the "headers" curl_slist, so that we can formulate it ourselves
    //
    const char* contentType = (orionldState.in.contentType == JSON)? "Content-Type: application/json" : "Content-Type: application/ld+json";
    headers = curl_slist_append(headers, contentType);

    if (orionldState.in.contentType == JSONLD)
    {
      // Add the context to the body if not there already
      KjNode* contextP = kjLookup(fwdPendingP->body, "@context");

      if (contextP == NULL)
      {
        LM(("Adding @context '%s' to body of msg to be forwarded", fwdPendingP->regP->contextP->url));
        contextP = kjString(orionldState.kjsonP, "@context", fwdPendingP->regP->contextP->url);
        kjChildAdd(fwdPendingP->body, contextP);
      }
    }

    approxContentLen = kjFastRenderSize(fwdPendingP->body);
    payloadBody      = kaAlloc(&orionldState.kalloc, approxContentLen);

    kjFastRender(fwdPendingP->body, payloadBody);

    //
    // Content-Length
    //
    int  contentLen = strlen(payloadBody);
    char contentLenHeader[64];
    snprintf(contentLenHeader, sizeof(contentLenHeader), "Content-Length: %d", contentLen);
    headers = curl_slist_append(headers, contentLenHeader);

    // BODY
    curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_POSTFIELDS, payloadBody);
  }


//
  // Need to save the pointer to the curl headers in order to free it afterwards
  //
  fwdPendingP->curlHeaders = headers;

  //
  // CURL Options
  //
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_CUSTOMREQUEST, orionldState.verbString);
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_TIMEOUT_MS, 5000);                     // Timeout - hard-coded to 5 seconds for now ...
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_FAILONERROR, true);                    // Fail On Error - to detect 404 etc.
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_FOLLOWLOCATION, 1L);                   // Follow redirections


  //
  // Reading the response
  //
  HttpResponse* httpResponseP = (HttpResponse*) kaAlloc(&orionldState.kalloc, sizeof(HttpResponse));

  httpResponseP->fwdPendingP  = fwdPendingP;
  httpResponseP->curlHandle   = fwdPendingP->curlHandle;
  httpResponseP->buf          = httpResponseP->preBuf;
  httpResponseP->bufPos       = 0;
  httpResponseP->bufLen       = sizeof(httpResponseP->preBuf);
  httpResponseP->mustBeFreed  = false;

  httpResponseP->fwdPendingP->rawResponse = httpResponseP->buf;

  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_WRITEFUNCTION, responseSave);          // Callback for reading the response body (and headers?)
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_WRITEDATA, (void*) httpResponseP);     // User data for responseSave

  if (https)
  {
    // SSL options
    curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);                 // ignore self-signed certificates for SSL end-points
    curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);                 // DO NOT verify the certificate's name against host
  }

  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_HTTPHEADER, headers);

  //
  // SEND (sort of)
  //
  curl_multi_add_handle(orionldState.curlFwdMultiP, fwdPendingP->curlHandle);

  LM(("FWD: Request '%s' has been enqueued for forwarding", url));
  return 0;
}
