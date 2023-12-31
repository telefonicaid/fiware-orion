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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
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
  if (valueInSubCache <= 0.1)
  {
    KjNode* timestampNodeP = kjLookup(apiSubP, fieldName);
    if (timestampNodeP != NULL)
      kjChildRemove(apiSubP, timestampNodeP);
    return;
  }

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
    timestampNodeP->type    = KjString;
    timestampNodeP->value.s = kaStrdup(&orionldState.kalloc, dateTime);
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
// Three options:
//   1. cSubP given
//   2. pSubP given
//   3, None of them (look up apiSubP::subscriptionId in both cashes)
//
void orionldSubCounters(KjNode* apiSubP, CachedSubscription* cSubP, PernotSubscription* pSubP)
{
  KjNode* notificationP = kjLookup(apiSubP, "notification");

  if (notificationP == NULL)
    LM_RVE(("API Subscription without a notification field !!!"));

  if ((cSubP == NULL) && (pSubP == NULL))
  {
    KjNode* subIdP = kjLookup(apiSubP, "id");

    if (subIdP == NULL)
      return;

    cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subIdP->value.s);

    if (cSubP == NULL)
    {
      pSubP = pernotSubCacheLookup(orionldState.tenantP->tenant, subIdP->value.s);
      if (pSubP == NULL)
        LM_RVE(("Can't find subscription '%s' in any subscription cache", subIdP->value.s));
    }
  }

  double  lastNotificationTime = (cSubP != NULL)? cSubP->lastNotificationTime : pSubP->lastNotificationTime;
  double  lastSuccess          = (cSubP != NULL)? cSubP->lastSuccess          : pSubP->lastSuccessTime;
  double  lastFailure          = (cSubP != NULL)? cSubP->lastFailure          : pSubP->lastFailureTime;
  int     timesSent            = 0;
  int     timesFailed          = 0;
  int     noMatch              = 0;

  if (cSubP != NULL)
  {
    timesSent   = cSubP->dbCount    + cSubP->count;
    timesFailed = cSubP->dbFailures + cSubP->failures;
  }
  else
  {
    timesSent   = pSubP->notificationAttempts + pSubP->notificationAttemptsDb;
    timesFailed = pSubP->notificationErrors   + pSubP->notificationErrorsDb;
    noMatch     = pSubP->noMatch              + pSubP->noMatchDb;
  }
  //
  // Set the values
  //
  subTimestampSet(notificationP, "lastNotification", lastNotificationTime);
  subTimestampSet(notificationP, "lastSuccess",      lastSuccess);
  subTimestampSet(notificationP, "lastFailure",      lastFailure);
  subCounterSet(notificationP,   "timesSent",        timesSent);
  subCounterSet(notificationP,   "timesFailed",      timesFailed);
  subCounterSet(notificationP,   "noMatch",          noMatch);
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
  kjTreeLog(dbSubP, "DB Sub", LmtSubCacheStats);

  KjNode*             coordinatesNodeP = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*             contextNodeP     = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*             showChangesP     = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  KjNode*             sysAttrsP        = NULL;           // Not needed here, but dbModelToApiSubscription requires it
  OrionldRenderFormat renderFormat     = RF_NORMALIZED;  // Not needed here, but dbModelToApiSubscription requires it
  double              timeInterval     = 0;
  KjNode*             apiSubP          = dbModelToApiSubscription(dbSubP,
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
    PernotSubscription* pSubP = pernotSubCacheLookup(orionldState.tenantP->tenant, orionldState.wildcard[0]);
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

  char* subscriptionId = orionldState.wildcard[0];

  // "Normal" (onchange) subscription?
  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);
  if (cSubP != NULL)
  {
    orionldState.httpStatusCode = 200;

    orionldState.responseTree   = kjTreeFromCachedSubscription(cSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == MT_JSONLD);
    orionldSubCounters(orionldState.responseTree, cSubP, NULL);

    return true;
  }

  // pernot subscription?
  PernotSubscription* pSubP = pernotSubCacheLookup(orionldState.tenantP->tenant, subscriptionId);
  if (pSubP != NULL)
  {
    orionldState.httpStatusCode = 200;

    orionldState.responseTree   = kjTreeFromPernotSubscription(pSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == MT_JSONLD);
    orionldSubCounters(orionldState.responseTree, NULL, pSubP);

    return true;
  }

  orionldError(OrionldResourceNotFound, "subscription not found", subscriptionId, 404);
  return false;
}
