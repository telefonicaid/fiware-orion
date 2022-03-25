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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"

#include "cache/subCache.h"                                      // CachedSubscription
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/orionldServerConnect.h"                 // orionldServerConnect
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlterationMatch, OrionldAlteration, orionldAlterationType
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



extern void patchApply(KjNode* patchBase, KjNode* patchP);  // FIXME: Move to separate module  orionld/?common?/orionldPatchApply.h/cpp
// -----------------------------------------------------------------------------
//
// notificationBody -
//
static KjNode* notificationBody(OrionldAlterationMatch* mAltP, bool contextInBody)
{
  KjNode* notificationTree = kjObject(orionldState.kjsonP, NULL);
  char    notificationId[80];

  strncpy(notificationId, "urn:ngsi-ld:Notification:", sizeof(notificationId) - 1);  // notificationId, could be a thread variable ...
  uuidGenerate(&notificationId[25], sizeof(notificationId) - 25, false);

  KjNode* idNodeP              = kjString(orionldState.kjsonP, "id", notificationId);
  KjNode* typeNodeP            = kjString(orionldState.kjsonP, "type", "Notification");
  KjNode* subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", mAltP->subP->subscriptionId);
  KjNode* notifiedAtNodeP      = kjString(orionldState.kjsonP, "notifiedAt", orionldState.requestTimeString);
  KjNode* dataNodeP            = kjArray(orionldState.kjsonP,  "data");

  kjChildAdd(notificationTree, idNodeP);
  kjChildAdd(notificationTree, typeNodeP);
  kjChildAdd(notificationTree, subscriptionIdNodeP);
  kjChildAdd(notificationTree, notifiedAtNodeP);
  kjChildAdd(notificationTree, dataNodeP);

  if (mAltP->altP->entityP == NULL)
  {
    //
    // Merge dbEntityP + patchTree
    //
    mAltP->altP->entityP = dbModelToApiEntity(mAltP->altP->dbEntityP, false, mAltP->altP->entityId);
    if (mAltP->altP->entityP == NULL)
    {
      LM_E(("Internal Error (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
      return NULL;
    }

    for (KjNode* patchP = mAltP->altP->patchTree->value.firstChildP; patchP != NULL; patchP = patchP->next)
    {
      patchApply(mAltP->altP->entityP, patchP);
    }

    // eqForDot + compact attribute names
    for (KjNode* attrP = mAltP->altP->entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      // FIXME: Need to use subP->ldContext

      if (strcmp(attrP->name, "id")         == 0) continue;
      if (strcmp(attrP->name, "createdAt")  == 0) continue;
      if (strcmp(attrP->name, "modifiedAt") == 0) continue;

      // compaction of the value of 'type'
      if (strcmp(attrP->name, "type") == 0)
      {
        attrP->value.s = orionldContextItemAliasLookup(orionldState.contextP, attrP->value.s, NULL, NULL);
        continue;
      }

      eqForDot(attrP->name);
      attrP->name = orionldContextItemAliasLookup(orionldState.contextP, attrP->name, NULL, NULL);
    }

    if (contextInBody)
    {
      KjNode* contextNodeP = kjString(orionldState.kjsonP, "@context", orionldState.contextP->url);  // FIXME: use context from subscription!
      kjChildAdd(mAltP->altP->entityP, contextNodeP);
    }
  }

  kjChildAdd(dataNodeP, mAltP->altP->entityP);

  return notificationTree;
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
void notificationSend(OrionldAlterationMatch* altP)
{
  //
  // Preparing the HTTP headers which will be pretty much the same for all notifications
  // What differs is Content-Length, Content-Type, and the Request header
  //
  char  requestHeader[128];
  char  contentLenHeader[32];
  char* lenP = &contentLenHeader[16];

  snprintf(requestHeader, sizeof(requestHeader), "POST /%s HTTP/1.1\r\n", altP->subP->rest);  // The slash is needed as it was removed in "urlParse" in subCache.cpp
  strcpy(contentLenHeader, "Content-Length: 0");  // Can't modify inside static strings, so need a char-vec on the stack for contentLenHeader

  KjNode* bodyP = notificationBody(altP, altP->subP->httpInfo.mimeType == JSONLD);
  if (bodyP == NULL)
  {
    LM_E(("Internal Error (NULL notification body - can't notify)"));
    return;
  }

  int     bodyBufSize = kjFastRenderSize(bodyP);
  char*   bodyBuf     = (bodyBufSize < (int) sizeof(body))? body : kaAlloc(&orionldState.kalloc, bodyBufSize);

  // if bodyBufSize is bigger than the kjAlloc extra size, we'll need to use malloc ...

  kjFastRender(bodyP, bodyBuf);

  int               sizeLeftForLen = 16;               // 16: sizeof(contentLenHeader) - 16
  long unsigned int contentLength  = strlen(bodyBuf);  // FIXME: kjFastRender should return the size
  char              linkHeader[512];

  snprintf(lenP, sizeLeftForLen, "%d\r\n", (int) contentLength);  // Adding Content-Length inside contentLenHeader

  struct iovec  ioVec[6] = {
    { requestHeader,    0 },
    { contentLenHeader, 0 },
    { (void*) contentTypeHeaderJson, 32 },
    { (void*) userAgentHeader,       23 },
    { (void*) "\r\n", 2},
    { bodyBuf, contentLength }
  };
  int ioVecLen = 6;

  if (altP->subP->httpInfo.mimeType == JSONLD)
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderJsonLd;
    ioVec[2].iov_len  = 35;
  }
  else  // Add Link header
  {
    // Replace ioVec[4] - make sure to end it in double newline/linefeed
    snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"\r\n\r\n", altP->subP->ldContext.c_str());
    ioVec[4].iov_base = linkHeader;
    ioVec[4].iov_len  = strlen(linkHeader);
  }

  LM_TMP(("KZ: Notification:"));
  for (int ix = 0; ix < ioVecLen; ix++)
  {
    LM_TMP(("KZ:   %s", (char*) ioVec[ix].iov_base));
  }
}
