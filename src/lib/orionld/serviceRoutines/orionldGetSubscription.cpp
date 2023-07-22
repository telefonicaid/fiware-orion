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
#include <string.h>                                              // strncpy

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjInteger, kjChildAdd
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "cache/subCache.h"                                      // CachedSubscription, subCacheItemLookup
#include "orionld/pernot/pernotSubCacheLookup.h"                 // pernotSubCacheLookup
#include "orionld/legacyDriver/legacyGetSubscription.h"          // legacyGetSubscription
#include "orionld/kjTree/kjTreeFromCachedSubscription.h"         // kjTreeFromCachedSubscription
#include "orionld/kjTree/kjTreeFromPernotSubscription.h"         // kjTreeFromPernotSubscription
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/dbModel/dbModelToApiSubscription.h"            // dbModelToApiSubscription
#include "orionld/mongoc/mongocSubscriptionLookup.h"             // mongocSubscriptionLookup
#include "orionld/serviceRoutines/orionldGetSubscription.h"      // Own Interface



// -----------------------------------------------------------------------------
//
// subTimestampSet -
//
static void subTimestampSet(KjNode* apiSubP, const char* fieldName, double valueInSubCache)
{
  if (valueInSubCache <= 0)
    return;

  KjNode* timestampNodeP = kjLookup(apiSubP, fieldName);

  char dateTime[64];
  numberToDate(valueInSubCache, dateTime, sizeof(dateTime) - 1);

  if (timestampNodeP == NULL)  // Doesn't exist, we'll have to create it
  {
    timestampNodeP = kjString(orionldState.kjsonP, fieldName, dateTime);
    kjChildAdd(apiSubP, timestampNodeP);
  }
  else
  {
    // We have to assume the current timestamp has a char buffer that is of sufficient length.
    // We actually know that, as the string was initially created as a timestamp
    strncpy(timestampNodeP->value.s, dateTime, 64);
  }
}



// -----------------------------------------------------------------------------
//
// subCounterSet -
//
static void subCounterSet(KjNode* apiSubP, const char* fieldName, int64_t valueInSubCache)
{
  if (valueInSubCache <= 0)
    return;

  KjNode* counterNodeP = kjLookup(apiSubP, fieldName);

  if (counterNodeP == NULL)  // Doesn't exist, we'll have to create it
  {
    counterNodeP = kjInteger(orionldState.kjsonP, fieldName, valueInSubCache);
    kjChildAdd(apiSubP, counterNodeP);
  }
  else
    counterNodeP->value.i = valueInSubCache;
}



// -----------------------------------------------------------------------------
//
// orionldSubCounters - FIXME: Own Module
//
void orionldSubCounters(KjNode* apiSubP, CachedSubscription* cSubP, PernotSubscription* pSubP)
{
  //
  // Three options:
  // 1. cSubP given
  // 2. pSubP given
  // 3, None of them (look up apiSubP::subscriptionId in both cashes)
  //
  double lastNotificationTime = -1;
  double lastSuccess          = -1;
  double lastFailure          = -1;
  int    timesSent            = -1;

  if ((cSubP == NULL) && (pSubP == NULL))
  {
    KjNode* subIdP = kjLookup(apiSubP, "id");

    if (subIdP == NULL)
      return;

    cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subIdP->value.s);
    pSubP = pernotSubCacheLookup(orionldState.tenantP->tenant, subIdP->value.s);
    if ((cSubP == NULL) && (pSubP == NULL))
      LM_RVE(("Can't find subscription '%s'", subIdP->value.s));
  }

  if (cSubP != NULL)
  {
    lastNotificationTime = cSubP->lastNotificationTime;
    lastSuccess          = cSubP->lastSuccess;
    lastFailure          = cSubP->lastFailure;
    timesSent            = cSubP->dbCount + cSubP->count;
  }
  else if (pSubP != NULL)
  {
    lastNotificationTime = pSubP->lastNotificationAttempt;
    lastSuccess          = pSubP->lastSuccessTime;
    lastFailure          = pSubP->lastFailureTime;
    timesSent            = pSubP->dbNotificationAttempts + pSubP->notificationAttempts;
  }

  //
  // Set the values
  //
  KjNode* notificationNodeP = kjLookup(apiSubP, "notification");
  if (notificationNodeP != NULL)
  {
    subTimestampSet(notificationNodeP, "lastNotification", lastNotificationTime);
    subTimestampSet(notificationNodeP, "lastSuccess",      lastSuccess);
    subTimestampSet(notificationNodeP, "lastFailure",      lastFailure);
    subCounterSet(notificationNodeP,   "timesSent",        timesSent);
  }
}



// ----------------------------------------------------------------------------
//
// orionldGetSubscriptionFromDb -
//
// Some of the data comes from the subscription cache, like counters and timestamps
//
static bool orionldGetSubscriptionFromDb(void)
{
  KjNode* dbSubP = mongocSubscriptionLookup(orionldState.wildcard[0]);

  if (dbSubP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Subscription Not Found", orionldState.wildcard[0], 404);
    return false;
  }

  KjNode*      coordinatesNodeP = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*      contextNodeP     = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*      showChangesP     = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*      sysAttrsP        = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  RenderFormat renderFormat     = RF_NORMALIZED;  // Not needed here, but dbModelToApiSubscription requires it
  double       timeInterval     = 0;
  KjNode*      apiSubP          = dbModelToApiSubscription(dbSubP,
                                                           orionldState.tenantP->tenant,
                                                           false,
                                                           NULL,
                                                           &coordinatesNodeP,
                                                           &contextNodeP,
                                                           &showChangesP,
                                                           &sysAttrsP,
                                                           &renderFormat,
                                                           &timeInterval);

  if (apiSubP == NULL)
  {
    orionldError(OrionldInternalError, "Internal Error", "unable to convert DB-Subscription into API-Subscription", 500);
    return false;
  }

  // Need to take counters and timestamps from sub-cache
  if (timeInterval == 0)
  {
    CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, orionldState.wildcard[0]);
    if (cSubP != NULL)
      orionldSubCounters(apiSubP, cSubP, NULL);
  }
  else
  {
    PernotSubscription* pSubP = pernotSubCacheLookup(orionldState.wildcard[0], orionldState.tenantP->tenant);
    if (pSubP != NULL)
      orionldSubCounters(apiSubP, NULL, pSubP);
  }

  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = apiSubP;

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldGetSubscription -
//
bool orionldGetSubscription(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyGetSubscription();

  // Is orionldState.wildcard[0] a valid id for a subscription?
  PCHECK_URI(orionldState.wildcard[0], true, 0, NULL, "Invalid Subscription ID", 400);

  if (orionldState.uriParamOptions.fromDb == true)
  {
    //
    // GET Subscription with mongoc
    //
    return orionldGetSubscriptionFromDb();
  }

  char*                 subscriptionId = orionldState.wildcard[0];

  // "Normal" (onchange) subscription?
  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);
  if (cSubP != NULL)
  {
    orionldState.httpStatusCode = 200;
    orionldState.responseTree   = kjTreeFromCachedSubscription(cSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == JSONLD);

    return true;
  }

  // pernot subscription?
  PernotSubscription* pSubP = pernotSubCacheLookup(subscriptionId, orionldState.tenantP->tenant);
  if (pSubP != NULL)
  {
    kjTreeLog(pSubP->kjSubP, "pernot sub in cache", LmtSR);
    orionldState.httpStatusCode = 200;
    orionldState.responseTree   = kjTreeFromPernotSubscription(pSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == JSONLD);
    kjTreeLog(orionldState.responseTree, "pernot sub after kjTreeFromPernotSubscription", LmtSR);

    return true;
  }

  orionldError(OrionldResourceNotFound, "subscription not found", subscriptionId, 404);
  return false;
}
