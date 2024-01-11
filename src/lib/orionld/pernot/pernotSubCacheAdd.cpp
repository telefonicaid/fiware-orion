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
#include <stdlib.h>                                            // malloc
#include <semaphore.h>                                         // sem_post

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/Protocol.h"                            // Protocol, protocolFromString
#include "orionld/types/OrionldTenant.h"                       // OrionldTenant
#include "orionld/types/QNode.h"                               // QNode
#include "orionld/types/PernotSubscription.h"                  // PernotSubscription
#include "orionld/types/PernotSubCache.h"                      // PernotSubCache
#include "orionld/types/OrionldContext.h"                      // OrionldContext
#include "orionld/types/OrionldRenderFormat.h"                 // OrionldRenderFormat
#include "orionld/common/orionldState.h"                       // orionldState, pernotSubCache
#include "orionld/common/urlParse.h"                           // urlParse
#include "orionld/payloadCheck/pcheckGeoQ.h"                   // pcheckGeoQ
#include "orionld/kjTree/kjChildCount.h"                       // kjChildCount



// -----------------------------------------------------------------------------
//
// receiverInfo -
//
// As there is no incoming info for the HTTP headers of Pernot subscriptions, we can
// safely create the entire string of 'key: value' for all the headers.
//
static void receiverInfo(PernotSubscription* pSubP, KjNode* endpointP)
{
  KjNode* receiverInfoP = kjLookup(endpointP, "receiverInfo");

  pSubP->headers.items = 0;

  if (receiverInfoP != NULL)
  {
    pSubP->headers.items = kjChildCount(receiverInfoP);
    pSubP->headers.array = (char**) malloc(pSubP->headers.items * sizeof(char*));

    int ix = 0;
    for (KjNode* kvPairP = receiverInfoP->value.firstChildP; kvPairP != NULL; kvPairP = kvPairP->next)
    {
      KjNode* keyP   = kjLookup(kvPairP, "key");
      KjNode* valueP = kjLookup(kvPairP, "value");
      int     sLen   = strlen(keyP->value.s) + strlen(valueP->value.s) + 4;
      char*   s      = (char*) malloc(sLen);

      snprintf(s, sLen - 1, "%s:%s\r\n", keyP->value.s, valueP->value.s);
      pSubP->headers.array[ix] = s;
      ++ix;
    } 
  }
}



// -----------------------------------------------------------------------------
//
// counterFromDb -
//
static void counterFromDb(KjNode* subP, uint32_t* counterP, const char* fieldName)
{
  LM_T(LmtPernot, ("Getting counter '%s' from db", fieldName));
  KjNode* counterNodeP = kjLookup(subP, fieldName);

  if (counterNodeP != NULL)
  {
    LM_T(LmtPernot, ("Found counter '%s' in db: %d", fieldName, counterNodeP->value.i));
    *counterP = counterNodeP->value.i;
  }
  else
  {
    *counterP = 0;
    LM_T(LmtPernot, ("Counter '%s' NOT found in db", fieldName));
  }
}



// -----------------------------------------------------------------------------
//
// timestampFromDb -
//
static void timestampFromDb(KjNode* apiSubP, double* tsP, const char* fieldName)
{
  KjNode* tsNodeP = kjLookup(apiSubP, fieldName);
  if (tsNodeP != NULL)
  {
    // Need to remove it from apiSubP and save it in cache
    kjChildRemove(apiSubP, tsNodeP);
    *tsP = tsNodeP->value.f;
  }
  else
    *tsP = 0;
}



// -----------------------------------------------------------------------------
//
// pernotSubCacheAdd -
//
PernotSubscription* pernotSubCacheAdd
(
  char*                subscriptionId,
  KjNode*              apiSubP,
  KjNode*              endpointP,
  QNode*               qTree,
  KjNode*              geoCoordinatesP,
  OrionldContext*      contextP,
  OrionldTenant*       tenantP,
  KjNode*              showChangesP,
  KjNode*              sysAttrsP,
  OrionldRenderFormat  renderFormat,
  double               timeInterval
)
{
  PernotSubscription* pSubP = (PernotSubscription*) malloc(sizeof(PernotSubscription));
  bzero(pSubP, sizeof(PernotSubscription));

  LM_T(LmtPernot, ("Creating pernot subscription %s (at %p)", subscriptionId, pSubP));

  if (subscriptionId == NULL)
  {
    KjNode* idP = kjLookup(apiSubP, "id");
    if (idP == NULL)
      LM_RE(NULL, ("No subscription id"));
    subscriptionId = idP->value.s;
  }

  LM_T(LmtPernot, ("Adding pernot subscription '%s' to cache (top level name: '%s')", subscriptionId, apiSubP->name));

  pSubP->subscriptionId = strdup(subscriptionId);
  pSubP->timeInterval   = timeInterval;
  pSubP->kjSubP         = kjClone(NULL, apiSubP);
  LM_T(LmtLeak, ("Cloned an apiSubP: %p", pSubP->kjSubP));
  kjTreeLog(pSubP->kjSubP, "apiSubP", LmtLeak);
  pSubP->tenantP        = tenantP;
  pSubP->renderFormat   = renderFormat;
  pSubP->sysAttrs       = (sysAttrsP == NULL)? false : sysAttrsP->value.b;
  pSubP->ngsiv2         = (renderFormat >= RF_CROSS_APIS_NORMALIZED);
  pSubP->context        = (contextP == NULL)? NULL : contextP->url;
  pSubP->isActive       = true;  // Active by default, then we'll see ...

  KjNode* notificationP = kjLookup(pSubP->kjSubP, "notification");

  // Query parameters
  pSubP->eSelector      = kjLookup(pSubP->kjSubP, "entities");
  pSubP->attrsSelector  = kjLookup(notificationP, "attributes");
  pSubP->qSelector      = qTree;
  pSubP->geoSelector    = NULL;

  KjNode* geoqP = kjLookup(apiSubP, "geoQ");
  if (geoqP != NULL)
  {
    pSubP->geoSelector = pcheckGeoQ(NULL, geoqP, true);  // FIXME: Already done in pcheckSubscription()
    if (pSubP->geoSelector != NULL)
    {
      if (pSubP->geoSelector->geoProperty == NULL)
        pSubP->geoSelector->geoProperty = (char*) "location";

      LM_T(LmtPernotQuery, ("geometry:    %d", pSubP->geoSelector->geometry));
      LM_T(LmtPernotQuery, ("georel:      %d", pSubP->geoSelector->georel));
      LM_T(LmtPernotQuery, ("minDistance: %d", pSubP->geoSelector->minDistance));
      LM_T(LmtPernotQuery, ("maxDistance: %d", pSubP->geoSelector->maxDistance));
      LM_T(LmtPernotQuery, ("geoProperty: '%s'", pSubP->geoSelector->geoProperty));
      pSubP->geoSelector->coordinates = kjClone(NULL, pSubP->geoSelector->coordinates);
      kjTreeLog(pSubP->geoSelector->coordinates, "coordinates", LmtPernotQuery);
      
    }
  }

  // notification::endpoint
  if (endpointP == NULL)
    endpointP = kjLookup(notificationP, "endpoint");
  
  // URL
  KjNode* uriP = kjLookup(endpointP, "uri");
  strncpy(pSubP->url, uriP->value.s, sizeof(pSubP->url) - 1);
  urlParse(pSubP->url, &pSubP->protocolString, &pSubP->ip, &pSubP->port, &pSubP->rest);
  pSubP->protocol = protocolFromString(pSubP->protocolString);

  // Mime Type for notifications
  KjNode*  acceptP  = kjLookup(endpointP, "accept");
  pSubP->mimeType = MT_JSON;  // Default setting
  if (acceptP != NULL)
  {
    if      (strcmp(acceptP->value.s, "application/json")     == 0) pSubP->mimeType = MT_JSON;
    else if (strcmp(acceptP->value.s, "application/ld+json")  == 0) pSubP->mimeType = MT_JSONLD;
    else if (strcmp(acceptP->value.s, "application/geo+json") == 0) pSubP->mimeType = MT_GEOJSON;
  }

  // HTTP headers from receiverInfo
  receiverInfo(pSubP, endpointP);

  // State - check also expiresAt+status
  KjNode* isActiveNodeP = kjLookup(apiSubP, "isActive");
  if (isActiveNodeP != NULL)
    pSubP->state = (isActiveNodeP->value.b == true)? SubActive : SubPaused;
  else
    pSubP->state = SubActive;

  // Counters
  counterFromDb(notificationP, &pSubP->notificationAttemptsDb, "timesSent");
  counterFromDb(notificationP, &pSubP->notificationErrorsDb,   "timesFailed");
  counterFromDb(notificationP, &pSubP->noMatchDb,              "noMatch");

  pSubP->notificationAttempts    = 0;  // cached - added to notificationAttemptsDb
  pSubP->notificationErrors      = 0;  // cached - added to notificationErrorsDb
  pSubP->noMatch                 = 0;  // cached - added to noMatchDb
  pSubP->consecutiveErrors       = 0;  // only cached, not saved in DB

  // Timestamps
  timestampFromDb(apiSubP, &pSubP->lastNotificationTime, "lastNotification");
  timestampFromDb(apiSubP, &pSubP->lastSuccessTime,      "lastSuccess");
  timestampFromDb(apiSubP, &pSubP->lastFailureTime,      "lastFailure");

  pSubP->cooldown                = 0;  // FIXME: get info from apiSubP
  pSubP->curlHandle              = NULL;

  //
  // Add the subscription to the cache
  //
  pSubP->next = NULL;

  // Take semaphore ...
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

  // notification::format
  KjNode* formatP = kjLookup(notificationP, "format");
  if (formatP == NULL)
  {
    formatP = kjString(NULL, "format", "normalized");
    kjChildAdd(notificationP, formatP);
  }

  // notification::endpoint::accept
  if (acceptP == NULL)
  {
    acceptP = kjString(NULL, "accept", "application/json");
    kjChildAdd(endpointP, acceptP);
  }

  //
  // Caching stuff from the KjNode tree
  //
  KjNode* langP = kjLookup(pSubP->kjSubP, "lang");
  pSubP->lang = (langP != NULL)? langP->value.s : NULL;

  return NULL;
}
