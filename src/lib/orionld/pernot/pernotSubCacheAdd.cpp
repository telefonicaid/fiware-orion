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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "common/RenderFormat.h"                               // RenderFormat

#include "orionld/common/orionldState.h"                       // pernotSubCache
#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/pernot/PernotSubscription.h"                 // PernotSubscription
#include "orionld/pernot/PernotSubCache.h"                     // PernotSubCache



// -----------------------------------------------------------------------------
//
// pernotSubCacheAdd -
//
PernotSubscription* pernotSubCacheAdd
(
  char*            subscriptionId,
  KjNode*          apiSubP,
  KjNode*          endpointP,
  QNode*           qTree,
  KjNode*          geoCoordinatesP,
  OrionldContext*  contextP,
  const char*      tenant,
  KjNode*          showChangesP,
  KjNode*          sysAttrsP,
  RenderFormat     renderFormat,
  double           timeInterval
)
{
  PernotSubscription* pSubP = (PernotSubscription*) malloc(sizeof(PernotSubscription));

  if (subscriptionId == NULL)
  {
    KjNode* idP = kjLookup(apiSubP, "id");
    if (idP == NULL)
      LM_RE(NULL, ("No subscription id"));
    subscriptionId = idP->value.s;
  }

  LM_T(LmtPernot, ("Adding pernot subscription '%s' to cache", subscriptionId));

  pSubP->subscriptionId = strdup(subscriptionId);
  pSubP->timeInterval   = timeInterval;
  pSubP->kjSubP         = kjClone(NULL, apiSubP);

  kjTreeLog(pSubP->kjSubP, "Initial Pernot Subscription", LmtPernot);

  if (tenant != NULL)
    strncpy(pSubP->tenant, tenant, sizeof(pSubP->tenant) - 1);

  // State - check also expiresAt+status
  KjNode* isActiveNodeP = kjLookup(apiSubP, "isActive");
  if (isActiveNodeP != NULL)
    pSubP->state = (isActiveNodeP->value.b == true)? SubActive : SubPaused;

  pSubP->lastNotificationAttempt = 0;  // FIXME: get info from apiSubP
  pSubP->lastSuccessTime         = 0;  // FIXME: get info from apiSubP
  pSubP->lastFailureTime         = 0;  // FIXME: get info from apiSubP
  pSubP->expiresAt               = 0;  // FIXME: get info from apiSubP
  pSubP->dbNotificationAttempts  = 0;  // FIXME: get info from apiSubP
  pSubP->notificationAttempts    = 0;
  pSubP->dbNotificationErrors    = 0;  // FIXME: get info from apiSubP
  pSubP->notificationErrors      = 0;
  pSubP->consecutiveErrors       = 0;  // FIXME: get info from apiSubP
  pSubP->cooldown                = 0;  // FIXME: get info from apiSubP
  pSubP->curlHandle              = NULL;

  //
  // Add the subscription to the cache
  //
  pSubP->next = NULL;

  if (pernotSubCache.head == NULL)
  {
    pernotSubCache.head = pSubP;
    pernotSubCache.tail = pSubP;
  }
  else
  {
    pernotSubCache.tail->next = pSubP;
    pernotSubCache.tail       = pSubP;
  }

  //
  // Prepare the kjSubP for future GET requests
  // Must add the "type", and, perhaps also the "jsonldContext"
  // Also, remove "q" and "mq" (that's NGSIv2),
  // and then rename "ldQ" to "q"
  // AND:
  //  isActive
  //  notification::endpoint::accept
  //  notification::format
  //  notification::status
  //  origin (cache)
  //  status
  //
  KjNode* typeP = kjString(NULL, "type", "Subscription");
  kjChildAdd(pSubP->kjSubP, typeP);

  KjNode* jsonldContextP = kjLookup(pSubP->kjSubP, "jsonldContext");
  if (jsonldContextP == NULL)
  {
    jsonldContextP = kjString(NULL, "jsonldContext", orionldState.contextP->url);
    kjChildAdd(pSubP->kjSubP, jsonldContextP);
  }

  KjNode* qP = kjLookup(pSubP->kjSubP, "q");
  if (qP != NULL)
    kjChildRemove(pSubP->kjSubP, qP);

  KjNode* mqP = kjLookup(pSubP->kjSubP, "mq");
  if (mqP != NULL)
    kjChildRemove(pSubP->kjSubP, mqP);

  qP = kjLookup(pSubP->kjSubP, "ldQ");
  if (qP != NULL)
    qP->name = (char*) "q";

  // origin
  KjNode* originP = kjString(NULL, "origin", "cache");
  kjChildAdd(pSubP->kjSubP, originP);

  // notification
  KjNode* notificationP = kjLookup(pSubP->kjSubP, "notification");

  // notification::format
  KjNode* formatP = kjLookup(notificationP, "format");
  if (formatP == NULL)
  {
    formatP = kjString(NULL, "format", "normalized");
    kjChildAdd(notificationP, formatP);
  }

  // notification::endpoint
  if (endpointP == NULL)
    endpointP = kjLookup(notificationP, "endpoint");
  
  // notification::endpoint::accept
  KjNode* acceptP = kjLookup(endpointP, "accept");
  if (acceptP == NULL)
  {
    acceptP = kjString(NULL, "accept", "application/json");
    kjChildAdd(endpointP, acceptP);
  }

  kjTreeLog(pSubP->kjSubP, "Pernot Subscription in Cache", LmtPernot);

  return NULL;
}
