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
#include "orionld/legacyDriver/legacyGetSubscription.h"          // legacyGetSubscription
#include "orionld/kjTree/kjTreeFromCachedSubscription.h"         // kjTreeFromCachedSubscription
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
    timestampNodeP = kjString(NULL, fieldName, dateTime);
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
    counterNodeP = kjInteger(NULL, fieldName, valueInSubCache);
    kjChildAdd(apiSubP, counterNodeP);
  }
  else
    counterNodeP->value.i = valueInSubCache;
}



// -----------------------------------------------------------------------------
//
// orionldSubCounters - FIXME: Own Module
//
void orionldSubCounters(KjNode* apiSubP, CachedSubscription* cSubP)
{
  if (cSubP == NULL)
  {
    KjNode* subIdP = kjLookup(apiSubP, "id");

    if (subIdP == NULL)
      return;

    cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subIdP->value.s);
    if (cSubP == NULL)
      return;
  }

  KjNode* notificationNodeP = kjLookup(apiSubP, "notification");

  if (notificationNodeP != NULL)
  {
    subTimestampSet(notificationNodeP, "lastNotification", cSubP->lastNotificationTime);
    subTimestampSet(notificationNodeP, "lastSuccess",      cSubP->lastSuccess);
    subTimestampSet(notificationNodeP, "lastFailure",      cSubP->lastFailure);
    subCounterSet(notificationNodeP,   "timesSent",        cSubP->dbCount + cSubP->count);
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

  KjNode* contextNodeP;      // Not used, but dbModelToApiSubscription requires it
  KjNode* coordinatesNodeP;  // Not used, but dbModelToApiSubscription requires it
  KjNode* apiSubP = dbModelToApiSubscription(dbSubP, orionldState.tenantP->tenant, false, NULL, &coordinatesNodeP, &contextNodeP);

  if (apiSubP == NULL)
  {
    orionldError(OrionldInternalError, "Internal Error", "unable to convert DB-Subscription into API-Subscription", 500);
    return false;
  }

  // Need to take counters and timestamps from sub-cache
  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, orionldState.wildcard[0]);

  if (cSubP != NULL)
    orionldSubCounters(apiSubP, cSubP);

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
  if (experimental == false)
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
  CachedSubscription*   cSubP          = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);

  if (cSubP != NULL)
  {
    orionldState.httpStatusCode = 200;
    orionldState.responseTree   = kjTreeFromCachedSubscription(cSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == JSONLD);

    return true;
  }

  orionldError(OrionldResourceNotFound, "subscription not found", subscriptionId, 404);
  return false;
}
