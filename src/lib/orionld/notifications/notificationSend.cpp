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

extern "C"
{
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray, kjString, kjChildAdd, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"

#include "cache/subCache.h"                                      // CachedSubscription
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/orionldServerConnect.h"                 // orionldServerConnect
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/orionldPatchApply.h"                    // orionldPatchApply
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlterationMatch, OrionldAlteration, orionldAlterationType
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/db/dbModelToApiEntity.h"                       // dbModelToApiEntity
#include "orionld/notifications/notificationSend.h"              // Own interface



// -----------------------------------------------------------------------------
//
// Fixed value headers
//
static const char* contentTypeHeaderJson   = (char*) "Content-Type: application/json\r\n";
static const char* contentTypeHeaderJsonLd = (char*) "Content-Type: application/ld+json\r\n";
static const char* userAgentHeader         = (char*) "User-Agent: orionld\r\n";



// -----------------------------------------------------------------------------
//
// static buffer for small notifications (payload body)
//
static __thread char body[4 * 1024];



// -----------------------------------------------------------------------------
//
// entityFix -
//
void entityFix(KjNode* entityP, OrionldContext* contextP)
{
  // eqForDot + compact attribute names
  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (strcmp(attrP->name, "id") == 0) continue;

    // compaction of the value of 'type'
    if (strcmp(attrP->name, "type") == 0)
    {
      attrP->value.s = orionldContextItemAliasLookup(contextP, attrP->value.s, NULL, NULL);
      continue;
    }

    eqForDot(attrP->name);
    attrP->name = orionldContextItemAliasLookup(contextP, attrP->name, NULL, NULL);
  }
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

  entityFix(entityP, subP->contextP);
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
int notificationSend(OrionldAlterationMatch* mAltP)
{
  LM_TMP(("Subscription '%s' is a match for update of entity '%s'", mAltP->subP->subscriptionId, mAltP->altP->entityId));

  KjNode* apiEntityP = dbModelToApiEntity(mAltP->altP->dbEntityP, false, mAltP->altP->entityId);  // No sysAttrs options for subscriptions?

  for (KjNode* patchP = mAltP->altP->patchTree->value.firstChildP; patchP != NULL; patchP = patchP->next)
  {
    orionldPatchApply(apiEntityP, patchP);
  }

  if (mAltP->subP->attributes.size() > 0)
  {
    LM_TMP(("Filter out attrs according to mAltP->subP->attributes"));
  }

  KjNode*            notificationP    = notificationTree(mAltP->subP, apiEntityP);
  long unsigned int  payloadBodySize  = kjFastRenderSize(notificationP);
  char*              payloadBody      = (payloadBodySize < sizeof(body))? body : kaAlloc(&orionldState.kalloc, payloadBodySize);

  kjFastRender(notificationP, payloadBody);


  //
  // Preparing the HTTP headers which will be pretty much the same for all notifications
  // What differs is Content-Length, Content-Type, and the Request header
  //
  char              requestHeader[128];
  char              contentLenHeader[32];
  char*             lenP           = &contentLenHeader[16];
  int               sizeLeftForLen = 16;                   // 16: sizeof(contentLenHeader) - 16
  long unsigned int contentLength  = strlen(payloadBody);  // FIXME: kjFastRender should return the size
  char              linkHeader[512];

  size_t requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s HTTP/1.1\r\n", mAltP->subP->rest);  // The slash is needed as it was removed in "urlParse" in subCache.cpp
  strcpy(contentLenHeader, "Content-Length: 0");  // Can't modify inside static strings, so need a char-vec on the stack for contentLenHeader

  snprintf(lenP, sizeLeftForLen, "%d\r\n", (int) contentLength);  // Adding Content-Length inside contentLenHeader

  struct iovec  ioVec[6] = {
    { requestHeader,                 requestHeaderLen },
    { contentLenHeader,              strlen(contentLenHeader) },
    { (void*) contentTypeHeaderJson, 32 },
    { (void*) userAgentHeader,       21 },
    { (void*) "\r\n",                2  },
    { payloadBody,                   contentLength }
  };
  int ioVecLen = 6;

  if (mAltP->subP->httpInfo.mimeType == JSONLD)
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderJsonLd;
    ioVec[2].iov_len  = 35;
  }
  else  // Add Link header
  {
    // Replace ioVec[4] - make sure to end it in double newline/linefeed
    snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"\r\n\r\n", mAltP->subP->ldContext.c_str());
    ioVec[4].iov_base = linkHeader;
    ioVec[4].iov_len  = strlen(linkHeader);
  }

  // <DEBUG>
  LM_TMP(("KZ: Notification:"));
  for (int ix = 0; ix < ioVecLen; ix++)
  {
    LM_TMP(("KZ:   %s", (char*) ioVec[ix].iov_base));
  }
  // </DEBUG>


  // Connect
  int fd = orionldServerConnect(mAltP->subP->ip, mAltP->subP->port);

  if (fd == -1)
  {
    LM_E(("Internal Error (unable to connent to server for notification for subscription '%s': %s)", mAltP->subP->subscriptionId, strerror(errno)));
    return -1;
  }
  LM_TMP(("Connected to %s:%d", mAltP->subP->ip, mAltP->subP->port));

  // Send
  int nb;
  if ((nb = writev(fd, ioVec, ioVecLen)) == -1)
  {
    close(fd);

    LM_E(("Internal Error (unable to send to server for notification for subscription '%s'): %s", mAltP->subP->subscriptionId, strerror(errno)));
    return -1;
  }

  LM_TMP(("Written %d bytes to fd %d of %s:%d for sub %s", nb, fd, mAltP->subP->ip, mAltP->subP->port, mAltP->subP->subscriptionId));
  return fd;
}
