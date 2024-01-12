/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/PernotSubscription.h"                    // PernotSubscription
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/context/orionldCoreContext.h"                  // ORIONLD_CORE_CONTEXT_URL_V1_0
#include "orionld/notifications/httpNotify.h"                    // httpNotify
#include "orionld/pernot/pernotSend.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// Fixed value headers - from notifications/notificationSend.cpp
//
extern const char* contentTypeHeaderJson;
extern const char* contentTypeHeaderJsonLd;
extern const char* contentTypeHeaderGeoJson;
extern const char* acceptHeader;
extern const char* normalizedHeader;
extern const char* conciseHeader;
extern const char* simplifiedHeader;
extern const char* normalizedHeaderNgsiV2;
extern const char* keyValuesHeaderNgsiV2;



// -----------------------------------------------------------------------------
//
// notificationTree -
//
static KjNode* notificationTree(PernotSubscription* subP, KjNode* entityArray)
{
  KjNode*             notificationP = kjObject(orionldState.kjsonP, NULL);
  char                notificationId[80];

  uuidGenerate(notificationId, sizeof(notificationId), "urn:ngsi-ld:Notification:");

  char date[64];
  numberToDate(subP->lastNotificationTime, date, sizeof(date));
  KjNode* idNodeP              = kjString(orionldState.kjsonP, "id",             notificationId);
  KjNode* typeNodeP            = kjString(orionldState.kjsonP, "type",           "Notification");
  KjNode* subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", subP->subscriptionId);
  KjNode* notifiedAtNodeP      = kjString(orionldState.kjsonP, "notifiedAt",     date);

  entityArray->name = (char*) "data";

  kjChildAdd(notificationP, idNodeP);
  kjChildAdd(notificationP, typeNodeP);
  kjChildAdd(notificationP, subscriptionIdNodeP);
  kjChildAdd(notificationP, notifiedAtNodeP);
  kjChildAdd(notificationP, entityArray);

  return notificationP;
}



// -----------------------------------------------------------------------------
//
// notificationTreeForNgsiV2 -
//
static KjNode* notificationTreeForNgsiV2(PernotSubscription* subP, KjNode* entityArray)
{
  LM_X(1, ("Pernot subs in NGSIv2 format is not implemented, how did we get here???"));
  return NULL;
}



// -----------------------------------------------------------------------------
//
// pernotSend -
//
bool pernotSend(PernotSubscription* subP, KjNode* entityArray)
{
  //
  // Outgoing Payload Body
  //
  char               body[2 * 1024];
  KjNode*            notificationP    = (subP->ngsiv2 == false)? notificationTree(subP, entityArray) : notificationTreeForNgsiV2(subP, entityArray);
  long unsigned int  payloadBodySize  = kjFastRenderSize(notificationP);
  char*              payloadBody      = (payloadBodySize < sizeof(body))? body : kaAlloc(&orionldState.kalloc, payloadBodySize);

  kjFastRender(notificationP, payloadBody);

  // Assuming HTTP for now

  //
  // Outgoing Header
  //
  char    requestHeader[512];
  size_t  requestHeaderLen = 0;

  if (subP->renderFormat < RF_CROSS_APIS_NORMALIZED)
    requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s?subscriptionId=%s HTTP/1.1\r\n",
                                subP->rest,
                                subP->subscriptionId);

  //
  // Content-Length
  //
  char   contentLenHeader[48];
  size_t contentLenHeaderLen;
  int    contentLen = strlen(payloadBody);

  contentLenHeaderLen = snprintf(contentLenHeader, sizeof(contentLenHeader) - 1, "Content-Length: %d\r\n", contentLen);


  int headers  = 7;  // the minimum number of request headers

  //
  // Headers from Subscription::notification::endpoint::receiverInfo+headers (or custom notification in NGSIv2 ...)
  //
  headers += subP->headers.items;

  char   hostHeader[512];
  size_t hostHeaderLen = snprintf(hostHeader, sizeof(hostHeader) - 1, "Host: %s:%d\r\n", subP->ip, subP->port);

  int           ioVecLen   = headers + 3;  // Request line + X headers + empty line + payload body
  struct iovec  ioVec[53]  = {
    { requestHeader,                 requestHeaderLen },
    { contentLenHeader,              contentLenHeaderLen },
    { (void*) contentTypeHeaderJson, 32 },  // Index 2
    { (void*) userAgentHeader,       userAgentHeaderLen },
    { (void*) hostHeader,            hostHeaderLen },
    { (void*) acceptHeader,          26 },
    { (void*) normalizedHeader,      37 }   // Index 6
  };
  int  headerIx      = 7;
  bool addLinkHeader = true;

  if (subP->mimeType == MT_JSONLD)  // If Content-Type is application/ld+json, modify slot 2 of ioVec
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderJsonLd;  // REPLACE "application/json" with "application/ld+json"
    ioVec[2].iov_len  = 35;
    addLinkHeader     = false;
  }

  if ((addLinkHeader == true) && (subP->ngsiv2 == false))  // Add Link header - but not if NGSIv2 Cross Notification
  {
    char         linkHeader[512];
    const char*  link = (subP->context == NULL)? ORIONLD_CORE_CONTEXT_URL_V1_0 : subP->context;

    snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"\r\n", link);

    ioVec[headerIx].iov_base = linkHeader;
    ioVec[headerIx].iov_len  = strlen(linkHeader);
    ++headerIx;
  }

  // FIXME: Loop over subP->headers.array, and add those to ioVec
  for (int ix = 0; ix < subP->headers.items; ix++)
  {
    ioVec[headerIx].iov_base = subP->headers.array[ix];
    ioVec[headerIx].iov_len  = strlen(subP->headers.array[ix]);
    ++headerIx;
  }

  //
  // Ngsild-Tenant
  //
  if ((subP->tenantP != NULL) && (subP->tenantP != &tenant0) && (subP->ngsiv2 == false))
  {
    int   len = strlen(subP->tenantP->tenant) + 20;  // Ngsild-Tenant: xxx\r\n0 - '\r' seems to not count for strlen ...
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "Ngsild-Tenant: %s\r\n", subP->tenantP->tenant);
    ioVec[headerIx].iov_base = buf;

    ++headerIx;
  }
    
  // Empty line delimiting HTTP Headers and Payload Body
  ioVec[headerIx].iov_base = (void*) "\r\n";
  ioVec[headerIx].iov_len  = 2;
  ++headerIx;


  // Payload Body
  ioVec[headerIx].iov_base = payloadBody;
  ioVec[headerIx].iov_len  = contentLen;

  ioVecLen = headerIx + 1;

  //
  // The message is ready - just need to be sent
  //
  if (subP->protocol == HTTP)
  {
    LM_T(LmtPernot, ("Sending a Periodic Notification to %s:%d%s", subP->ip, subP->port, subP->rest));
    return httpNotify(NULL, subP, subP->subscriptionId, subP->ip, subP->port, subP->rest, ioVec, ioVecLen, subP->lastNotificationTime);
  }
#if 0
  else if (subP->protocol == HTTPS)   return httpsNotify(subP, ioVec, ioVecLen, now, curlHandlePP);
  else if (subP->protocol == MQTT)    return mqttNotify(subP,  ioVec, ioVecLen, now);
#endif

  LM_W(("%s: Unsupported protocol for notifications: '%s'", subP->subscriptionId, subP->protocol));

  return false;
}
