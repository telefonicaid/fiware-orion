/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <string.h>                                              // strlen
#include <sys/uio.h>                                             // writev
#include <sys/select.h>                                          // select

#include <string>                                                // std::string (all because of receiverInfo!!!)
#include <map>                                                   // std::map    (all because of receiverInfo!!!)

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray, kjString, kjChildAdd, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"

#include "cache/subCache.h"                                      // CachedSubscription
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/orionldServerConnect.h"                 // orionldServerConnect
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlterationMatch, OrionldAlteration, orionldAlterationType
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/notifications/notificationSend.h"              // Own interface



// -----------------------------------------------------------------------------
//
// Fixed value headers
//
static const char* contentTypeHeaderJson   = (char*) "Content-Type: application/json\r\n";
static const char* contentTypeHeaderJsonLd = (char*) "Content-Type: application/ld+json\r\n";
static const char* userAgentHeader         = (char*) "User-Agent: orionld\r\n";
static const char* acceptHeader            = (char*) "Accept: application/json\r\n";

static const char* normalizedHeader        = (char*) "Ngsild-Attribute-Format: Normalized\r\n";
static const char* conciseHeader           = (char*) "Ngsild-Attribute-Format: Concise\r\n";
static const char* simplifiedHeader        = (char*) "Ngsild-Attribute-Format: Simplified\r\n";

static const char* normalizedHeaderNgsiV2  = (char*) "Ngsiv2-Attrsformat: normalized\r\n";
static const char* keyValuesHeaderNgsiV2   = (char*) "Ngsiv2-Attrsformat: keyValues\r\n";



// -----------------------------------------------------------------------------
//
// static buffer for small notifications (payload body)
//
static __thread char body[4 * 1024];



// -----------------------------------------------------------------------------
//
// attributeToSimplified - move to its own module
//
// 1. Find the type
// 2. Knowing the type, find the value ("value", "object", or "languageMap")
// 3. Make the value node the RHS of the attribute
//
static void attributeToSimplified(KjNode* attrP)
{
  // Get the attribute type
  KjNode* attrTypeP = kjLookup(attrP, "type");
  if (attrTypeP == NULL)
    LM_RVE(("Attribute '%s' has no type", attrP->name));
  if (attrTypeP->type != KjString)
    LM_RVE(("Attribute '%s' has a type that is not a JSON String", attrP->name));

  // Get the value
  char* valueFieldName = (char*) "value";
  if (strcmp(attrTypeP->value.s, "Relationship") == 0)
    valueFieldName = (char*) "object";
  else if (strcmp(attrTypeP->value.s, "LanguageProperty") == 0)
    valueFieldName = (char*) "languageMap";

  KjNode* valueP = kjLookup(attrP, valueFieldName);

  if (valueP == NULL)
    LM_RVE(("Attribute '%s' has no value '%s'", attrP->name, valueFieldName));

  attrP->type  = valueP->type;
  attrP->value = valueP->value;
}



// -----------------------------------------------------------------------------
//
// attributeToConcise - move to its own module
//
// 1. Find and remove the type
// 2. If only one item left and it's "value" - Simplified
//
static void attributeToConcise(KjNode* attrP, bool* simplifiedP)
{
  // Get the attribute type and remove it
  KjNode* attrTypeP = kjLookup(attrP, "type");
  if (attrTypeP == NULL)
    LM_RVE(("Attribute '%s' has no type", attrP->name));
  if (attrTypeP->type != KjString)
    LM_RVE(("Attribute '%s' has a type that is not a JSON String", attrP->name));

  kjChildRemove(attrP, attrTypeP);
  LM_TMP(("CONCISE: Just removed type '%s'", attrTypeP->value.s));

  if ((strcmp(attrTypeP->value.s, "Property") != 0) && (strcmp(attrTypeP->value.s, "GeoProperty") != 0))
    return;

  // If only one item left in attrP - Simplified
  if ((attrP->value.firstChildP != NULL) && (attrP->value.firstChildP->next == NULL))
  {
    attrP->type  = attrP->value.firstChildP->type;
    attrP->value = attrP->value.firstChildP->value;
    *simplifiedP = true;
  }
}



// -----------------------------------------------------------------------------
//
// entityFix -
//
void entityFix(KjNode* entityP, CachedSubscription* subP)
{
  kjTreeLog(entityP, "Fixing Entity");

  bool simplified = (subP->renderFormat == NGSI_LD_V1_KEYVALUES) || (subP->renderFormat == NGSI_V2_KEYVALUES);
  bool concise    = (subP->renderFormat == NGSI_LD_V1_CONCISE);

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (strcmp(attrP->name, "id") == 0)
      continue;

    // compaction of the value of 'type' for the Entity
    if (strcmp(attrP->name, "type") == 0)
    {
      LM_TMP(("KZ orionldContextItemAliasLookup(contextP:%p, type:'%s')", subP->contextP, attrP->value.s));
      attrP->value.s = orionldContextItemAliasLookup(subP->contextP, attrP->value.s, NULL, NULL);
      continue;
    }


    //
    // Here we're "in attributeFix"
    //

    //
    // Never mind "location", "observationSpace", and "operationSpace"
    // It is pobably faster to lookup their alias (and get the same back) as it is to
    // do three string-comparisons in every loop
    //
    eqForDot(attrP->name);
    attrP->name = orionldContextItemAliasLookup(subP->contextP, attrP->name, NULL, NULL);

    bool asSimplified = false;
    if (simplified)
    {
      attributeToSimplified(attrP);
      continue;
    }
    else if (concise)
      attributeToConcise(attrP, &asSimplified);

    if (asSimplified == false)
    {
      //
      // Here we're "in subAttributeFix"
      //

      for (KjNode* saP = attrP->value.firstChildP; saP != NULL; saP = saP->next)
      {
        if (strcmp(saP->name, "value")       == 0) continue;
        if (strcmp(saP->name, "object")      == 0) continue;
        if (strcmp(saP->name, "languageMap") == 0) continue;
        if (strcmp(saP->name, "type")        == 0) continue;
        if (strcmp(saP->name, "observedAt")  == 0) continue;
        if (strcmp(saP->name, "unitCode")    == 0) continue;

        eqForDot(saP->name);
        saP->name = orionldContextItemAliasLookup(subP->contextP, saP->name, NULL, NULL);

        if ((subP->renderFormat == NGSI_LD_V1_KEYVALUES) || (subP->renderFormat == NGSI_V2_KEYVALUES))
          attributeToSimplified(saP);
        else if (subP->renderFormat == NGSI_LD_V1_CONCISE)
          attributeToConcise(saP, &asSimplified);  // asSimplified is not used down here
      }
    }
  }
}



// -----------------------------------------------------------------------------
//
// orionldEntityToNgsiV2 -
//
KjNode* orionldEntityToNgsiV2(KjNode* entityP, bool keyValues, bool compact)
{
  KjNode* v2EntityP = kjClone(orionldState.kjsonP, entityP);

  // For all attributes, create a "metadata" object and move all sub-attributes inside
  for (KjNode* attrP = v2EntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if ((compact == true) && (strcmp(attrP->name, "type") == 0))
    {
      eqForDot(attrP->value.s);
      attrP->value.s = orionldContextItemAliasLookup(orionldState.contextP, attrP->value.s, NULL, NULL);
    }

    if (attrP->type != KjObject)  // attributes are objects, "id", "type", etc, are not
      continue;

    eqForDot(attrP->name);
    if (compact)
      attrP->name = orionldContextItemAliasLookup(orionldState.contextP, attrP->name, NULL, NULL);

    // Turn object, languageMap to 'value'
    KjNode* objectP      = kjLookup(attrP, "object");
    KjNode* languageMapP = kjLookup(attrP, "languageMap");

    if (objectP      != NULL)  objectP->name      = (char*) "value";
    if (languageMapP != NULL)  languageMapP->name = (char*) "value";

    if (keyValues)
    {
      KjNode* valueP = kjLookup(attrP, "value");

      if (valueP != NULL)
      {
        attrP->type  = valueP->type;
        attrP->value = valueP->value;
      }

      continue;
    }

    // Got an attribute
    // - create a "metadata" object for the attribute
    // - move all sub-attributes inside "metadata"
    KjNode* metadataObjectP = kjObject(orionldState.kjsonP, "metadata");
    KjNode* mdP = attrP->value.firstChildP;
    KjNode* next;

    while (mdP != NULL)
    {
      if (mdP->type != KjObject)
      {
        mdP = mdP->next;
        continue;
      }
      next = mdP->next;
      kjChildRemove(attrP, mdP);
      kjChildAdd(metadataObjectP, mdP);

      // Turn object, languageMap to 'value'
      KjNode* objectP      = kjLookup(mdP, "object");
      KjNode* languageMapP = kjLookup(mdP, "languageMap");

      if (objectP      != NULL)  objectP->name      = (char*) "value";
      if (languageMapP != NULL)  languageMapP->name = (char*) "value";

      eqForDot(mdP->name);
      if (compact)
        mdP->name = orionldContextItemAliasLookup(orionldState.contextP, mdP->name, NULL, NULL);

      mdP = next;
    }

    kjChildAdd(attrP, metadataObjectP);
  }

  return v2EntityP;
}



// -----------------------------------------------------------------------------
//
// notificationTreeForNgsiV2 -
//
KjNode* notificationTreeForNgsiV2(CachedSubscription* subP, KjNode* entityP)
{
  KjNode* notificationP        = kjObject(orionldState.kjsonP, NULL);
  KjNode* subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", subP->subscriptionId);
  KjNode* dataNodeP            = kjArray(orionldState.kjsonP,  "data");
  bool    keyValues            = false;
  bool    compact              = false;

  if ((subP->renderFormat == NGSI_LD_V1_V2_KEYVALUES) || (subP->renderFormat == NGSI_LD_V1_V2_KEYVALUES_COMPACT))
    keyValues = true;

  if ((subP->renderFormat == NGSI_LD_V1_V2_NORMALIZED_COMPACT) || (subP->renderFormat == NGSI_LD_V1_V2_KEYVALUES_COMPACT))
    compact = true;

  KjNode* ngsiv2EntityP = orionldEntityToNgsiV2(entityP, keyValues, compact);

  kjChildAdd(dataNodeP, ngsiv2EntityP);
  kjChildAdd(notificationP, dataNodeP);
  kjChildAdd(notificationP, subscriptionIdNodeP);

  return notificationP;
}



// -----------------------------------------------------------------------------
//
// notificationTree -
//
KjNode* notificationTree(CachedSubscription* subP, KjNode* entityP)
{
  KjNode* notificationP = kjObject(orionldState.kjsonP, NULL);
  char    notificationId[80];

  strncpy(notificationId, "urn:ngsi-ld:Notification:", sizeof(notificationId) - 1);  // notificationId, could be a thread variable ...
  uuidGenerate(&notificationId[25], sizeof(notificationId) - 25, false);

  KjNode* idNodeP              = kjString(orionldState.kjsonP, "id", notificationId);
  KjNode* typeNodeP            = kjString(orionldState.kjsonP, "type", "Notification");
  KjNode* subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", subP->subscriptionId);
  KjNode* notifiedAtNodeP      = kjString(orionldState.kjsonP, "notifiedAt", orionldState.requestTimeString);
  KjNode* dataNodeP            = kjArray(orionldState.kjsonP,  "data");

  kjChildAdd(notificationP, idNodeP);
  kjChildAdd(notificationP, typeNodeP);
  kjChildAdd(notificationP, subscriptionIdNodeP);
  kjChildAdd(notificationP, notifiedAtNodeP);
  kjChildAdd(notificationP, dataNodeP);

  entityFix(entityP, subP);
  kjChildAdd(dataNodeP, entityP);

  if (subP->httpInfo.mimeType == JSONLD)  // Add @context to the entity
  {
    KjNode* contextNodeP = kjString(orionldState.kjsonP, "@context", orionldState.contextP->url);  // FIXME: use context from subscription!
    kjChildAdd(notificationP, contextNodeP);
  }

  return notificationP;
}



// -----------------------------------------------------------------------------
//
// attributeFilter -
//
static KjNode* attributeFilter(KjNode* apiEntityP, OrionldAlterationMatch* mAltP)
{
  LM_TMP(("Filter out attrs according to mAltP->subP->attributes"));

  KjNode* filteredEntityP = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrP           = apiEntityP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    bool clone = false;
    if      (strcmp(attrP->name, "id")   == 0) clone = true;
    else if (strcmp(attrP->name, "type") == 0) clone = true;
    else
    {
      char dotName[512];
      strncpy(dotName, attrP->name, sizeof(dotName) - 1);
      eqForDot(dotName);

      for (int ix = 0; ix < (int) mAltP->subP->attributes.size(); ix++)
      {
        const char* attrName = mAltP->subP->attributes[ix].c_str();

        LM_TMP(("XY: Comparing '%s' and '%s'", dotName, attrName));
        if (strcmp(dotName, attrName) == 0)
        {
          clone = true;
          break;
        }
      }
    }

    if (clone)
    {
      KjNode* nodeP = kjClone(orionldState.kjsonP, attrP);
      kjChildAdd(filteredEntityP, nodeP);
    }

    attrP = next;
  }

  return filteredEntityP;
}



// -----------------------------------------------------------------------------
//
// notificationSend -
//
// writev is used for the notifications.
// The advantasge with writev is that is takes as input an array of buffers, mwaning there's no
// need to copy the entire payload into one single buffer:
//
// ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
//
// struct iovec
// {
//   void  *iov_base;    /* Starting address */
//   size_t iov_len;     /* Number of bytes to transfer */
// };
//
int notificationSend(OrionldAlterationMatch* mAltP, double timestamp)
{
  LM_TMP(("KZ: Subscription '%s': renderFormat: '%s'", mAltP->subP->subscriptionId, renderFormatToString(mAltP->subP->renderFormat)));
  LM_TMP(("KZ: contextP at %p", mAltP->subP->contextP));
  kjTreeLog(mAltP->altP->patchedEntity, "patchedEntity");
  bool    ngsiv2     = (mAltP->subP->renderFormat >= NGSI_LD_V1_V2_NORMALIZED);
  KjNode* apiEntityP = mAltP->altP->patchedEntity;

  LM_TMP(("Subscription '%s' is a match for update of entity '%s'", mAltP->subP->subscriptionId, mAltP->altP->entityId));

  //
  // Filter out unwanted attributes, if so requested (by the Subscription)
  //
  if (mAltP->subP->attributes.size() > 0)
    apiEntityP = attributeFilter(apiEntityP, mAltP);


  //
  // Payload Body
  //
  KjNode*            notificationP    = (ngsiv2 == false)? notificationTree(mAltP->subP, apiEntityP) : notificationTreeForNgsiV2(mAltP->subP, apiEntityP);
  long unsigned int  payloadBodySize  = kjFastRenderSize(notificationP);
  char*              payloadBody      = (payloadBodySize < sizeof(body))? body : kaAlloc(&orionldState.kalloc, payloadBodySize);

  kjFastRender(notificationP, payloadBody);


  //
  // Preparing the HTTP headers which will be pretty much the same for all notifications
  // What differs is Content-Length, Content-Type, and the Request header
  //

  //
  // Request Header
  //
  char              requestHeader[512];
  size_t            requestHeaderLen;

  // The slash before the URL (rest) is needed as it was removed in "urlParse" in subCache.cpp
  if (mAltP->subP->renderFormat < NGSI_LD_V1_V2_NORMALIZED)
    requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s?subscriptionId=%s HTTP/1.1\r\n",
                                mAltP->subP->rest,
                                mAltP->subP->subscriptionId);
  else
    requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s HTTP/1.1\r\n", mAltP->subP->rest);


  //
  // Content-Length
  //
  char              contentLenHeader[32];
  char*             lenP           = &contentLenHeader[16];
  int               sizeLeftForLen = 16;                   // 16: sizeof(contentLenHeader) - 16
  long unsigned int contentLength  = strlen(payloadBody);  // FIXME: kjFastRender should return the size

  strcpy(contentLenHeader, "Content-Length: 0");  // Can't modify inside static strings, so need a char-vec on the stack for contentLenHeader
  snprintf(lenP, sizeLeftForLen, "%d\r\n", (int) contentLength);  // Adding Content-Length inside contentLenHeader

  int headers  = 6;  // the minimum number of request headers


  //
  // Headers to be forwarded in notifications (taken from the request that provoked the notification)
  //
  if (orionldState.in.tenant        != NULL)    ++headers;
  if (orionldState.in.xAuthToken    != NULL)    ++headers;
  if (orionldState.in.authorization != NULL)    ++headers;


  //
  // Headers from Subscription::notification::endpoint::receiverInfo+headers (or custom notification in NGSIv2 ...)
  //
  headers += mAltP->subP->httpInfo.headers.size();
  headers += mAltP->subP->httpInfo.receiverInfo.size();


  // Let's limit the number of headers to 50
  if (headers > 50)
    LM_X(1, ("Too many HTTP headers (>50) for a Notification - to support that many, the broker needs a SW update and to be recompiled"));

  int           ioVecLen   = headers + 3;  // Request line + X headers + empty line + payload body
  int           headerIx   = 6;
  struct iovec  ioVec[53]  = {
    { requestHeader,                 requestHeaderLen },
    { contentLenHeader,              strlen(contentLenHeader) },
    { (void*) contentTypeHeaderJson, 32 },  // Index 2
    { (void*) userAgentHeader,       21 },
    { (void*) acceptHeader,          26 },
    { (void*) normalizedHeader,      37 }   // Index 5
  };

  //
  // Content-Type and Link
  //
  if (mAltP->subP->httpInfo.mimeType == JSONLD)  // If Content-Type is application/ld+json, modify slot 2 of ioVec
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderJsonLd;  // REPLACE "application/json" with "application/ld+json"
    ioVec[2].iov_len  = 35;
  }
  else if (ngsiv2 == false)  // Add Link header - but not if NGSIv2 Cross Notification
  {
    char         linkHeader[512];
    const char*  link = (mAltP->subP->ldContext == "")? ORIONLD_CORE_CONTEXT_URL_V1_0 : mAltP->subP->ldContext.c_str();

    snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"\r\n", link);

    ioVec[headerIx].iov_base = linkHeader;
    ioVec[headerIx].iov_len  = strlen(linkHeader);
    ++headerIx;
  }

  //
  // Ngsild-Attribute-Format / Ngsiv1-Attrsformat
  //
  if (mAltP->subP->renderFormat == NGSI_LD_V1_CONCISE)
  {
    ioVec[5].iov_base = (void*) conciseHeader;
    ioVec[5].iov_len  = 34;
  }
  else if ((mAltP->subP->renderFormat == NGSI_LD_V1_KEYVALUES) || (mAltP->subP->renderFormat == NGSI_V2_KEYVALUES))
  {
    ioVec[5].iov_base = (void*) simplifiedHeader;
    ioVec[5].iov_len  = 37;
  }
  else if ((mAltP->subP->renderFormat == NGSI_LD_V1_V2_NORMALIZED) || (mAltP->subP->renderFormat == NGSI_LD_V1_V2_NORMALIZED_COMPACT))
  {
    ioVec[5].iov_base = (void*) normalizedHeaderNgsiV2;
    ioVec[5].iov_len  = 32;
  }
  else if ((mAltP->subP->renderFormat == NGSI_LD_V1_V2_KEYVALUES) || (mAltP->subP->renderFormat == NGSI_LD_V1_V2_KEYVALUES_COMPACT))
  {
    ioVec[5].iov_base = (void*) keyValuesHeaderNgsiV2;
    ioVec[5].iov_len  = 31;
  }


  //
  // Ngsild-Tenant
  //
  if ((orionldState.in.tenant != NULL) && (ngsiv2 == false))
  {
    int   len = strlen(orionldState.in.tenant) + 20;  // Ngsild-Tenant: xxx\r\n0 - '\r' seems to not count for strlen ...
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "Ngsild-Tenant: %s\r\n", orionldState.in.tenant);
    ioVec[headerIx].iov_base = buf;

    ++headerIx;
  }


  //
  // X-Auth-Token
  //
  if (orionldState.in.xAuthToken != NULL)
  {
    int   len = strlen(orionldState.in.xAuthToken) + 20;  // X-Auth-Token: xxx\r\n0
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "X-Auth-Token: %s\r\n", orionldState.in.xAuthToken);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
  }


  //
  // Authorization
  //
  if (orionldState.in.authorization != NULL)
  {
    int   len = strlen(orionldState.in.authorization) + 20;  // Authorization: xxx\r\n0
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "Authorization: %s\r\n", orionldState.in.authorization);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
  }

  //
  // FIXME: Store headers in a better way - see issue #1095
  //
  for (std::map<std::string, std::string>::const_iterator it = mAltP->subP->httpInfo.headers.begin(); it != mAltP->subP->httpInfo.headers.end(); ++it)
  {
    const char* key    = it->first.c_str();
    const char* value  = it->second.c_str();
    int         len    = strlen(key) + strlen(value) + 10;
    char*       buf    = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "%s: %s\r\n", key, value);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
    LM_TMP(("HttpInfo header '%s': '%s'", key, value));
  }

  // receiverInfo
  for (unsigned int ix = 0; ix < mAltP->subP->httpInfo.receiverInfo.size(); ix++)
  {
    const char* key    = mAltP->subP->httpInfo.receiverInfo[ix]->key;
    const char* value  = mAltP->subP->httpInfo.receiverInfo[ix]->value;
    int         len    = strlen(key) + strlen(value) + 10;
    char*       buf    = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "%s: %s\r\n", key, value);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
    LM_TMP(("HttpInfo receiverInfo '%s': '%s'", key, value));
  }


  // Empty line delimiting HTTP Headers and Payload Body
  ioVec[headerIx].iov_base = (void*) "\r\n";
  ioVec[headerIx].iov_len  = 2;
  ++headerIx;


  // Payload Body
  ioVec[headerIx].iov_base = payloadBody;
  ioVec[headerIx].iov_len  = contentLength;

  ioVecLen = headerIx + 1;

  // <DEBUG>
  int len = 0;
  LM_TMP(("Notification (%d iovecs):", ioVecLen));
  for (int ix = 0; ix < ioVecLen; ix++)
  {
    LM_TMP((" %d/%d %s", ioVec[ix].iov_len, strlen((char*) ioVec[ix].iov_base), (char*) ioVec[ix].iov_base));
    len += ioVec[ix].iov_len;
  }
  // </DEBUG>


  // Connect
  LM_TMP(("Connecting to %s:%d", mAltP->subP->ip, mAltP->subP->port));
  int fd = orionldServerConnect(mAltP->subP->ip, mAltP->subP->port);

  if (fd == -1)
  {
    LM_E(("Internal Error (unable to connect to server for notification for subscription '%s': %s)", mAltP->subP->subscriptionId, strerror(errno)));
    subscriptionFailure(mAltP->subP, "Unable to connect to notification endpoint", timestamp);
    return -1;
  }
  LM_TMP(("Connected to %s:%d on fd %d", mAltP->subP->ip, mAltP->subP->port, fd));

  // Send
  int nb;
  if ((nb = writev(fd, ioVec, ioVecLen)) == -1)
  {
    close(fd);

    LM_E(("Internal Error (unable to send to server for notification for subscription '%s' (fd: %d): %s", mAltP->subP->subscriptionId, fd, strerror(errno)));
    subscriptionFailure(mAltP->subP, "Unable to write to notification endpoint", timestamp);
    return -1;
  }

  LM_TMP(("Written %d bytes to fd %d of %s:%d for sub %s", nb, fd, mAltP->subP->ip, mAltP->subP->port, mAltP->subP->subscriptionId));
  return fd;
}
