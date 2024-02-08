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
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

#include "logMsg/logMsg.h"                                       // LM_*
#include "cache/subCache.h"                                      // CachedSubscription

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/common/orionldTenantLookup.h"                  // orionldTenantLookup
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_WLOG
#include "orionld/mongoc/mongocSubCountersUpdate.h"              // Own interface



// -----------------------------------------------------------------------------
//
// mongocSubCountersUpdate -
//
// This function increments the field
//   - count
// while it overwrites "if bigger" three timestamps:
//   - lastNotification
//   - lastSuccess
//   - lastFailure
//
// The command looks like this:
//
// db.csubs.update({_id: <subscriptionId>}, { $inc: {"count": X}, $max: { "lastNotification": LN, "lastSuccess": LS, "lastFailure": LF }})
//
// E.g.:
// db.csubs.update({_id: "urn:ngsi-ld:subs:S1"}, {$inc: {"count": 1}, $max: {"lastSuccess": 14.24, "lastFailure": 15.31, "lastNotification": 15.31}})
//
// IMPORTANT NOTE:
//   As this function may be called outside of the "Request Threads", orionldState cannot be used!
//
void mongocSubCountersUpdate
(
  OrionldTenant*       tenantP,
  const char*          subscriptionId,
  bool                 ngsild,
  int32_t              deltaAttempts,
  int32_t              deltaFailures,
  int32_t              deltaNoMatch,
  double               lastNotificationTime,
  double               lastSuccess,
  double               lastFailure,
  bool                 forcedToPause
)
{
  if (tenantP == NULL)
    tenantP = &tenant0;

  mongoc_client_t*      connectionP    = mongoc_client_pool_pop(mongocPool);
  mongoc_collection_t*  subscriptionsP = mongoc_client_get_collection(connectionP, tenantP->mongoDbName, "csubs");
  bson_t                request;         // Entire request with count and timestamps to be updated
  bson_t                reply;
  bson_t                max;
  bson_t                selector;
  bson_t                inc;

  if (subscriptionsP == NULL)
    LM_X(1, ("mongoc_client_get_collection failed for 'csubs' collection on tenant '%s'", tenantP->mongoDbName));

  bson_init(&reply);
  bson_init(&selector);
  bson_init(&inc);
  bson_init(&max);
  bson_init(&request);

  // Selector - The _id is an OID if not NGSI-LD
  if (ngsild == true)
    bson_append_utf8(&selector, "_id", 3, subscriptionId, -1);
  else
  {
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, subscriptionId);
    bson_append_oid(&selector, "_id", 3, &oid);
  }

  // Attempts (timesSent)
  if (deltaAttempts > 0)
    bson_append_int32(&inc, "count", 5, deltaAttempts);

  // deltaFailures (timesFailed)
  if (deltaFailures > 0)
    bson_append_int32(&inc, "timesFailed", 11, deltaFailures);

  // deltaNoMatch (noMatch)
  if (deltaNoMatch > 0)
    bson_append_int32(&inc, "noMatch", 7, deltaNoMatch);

  if (deltaAttempts + deltaFailures + deltaNoMatch > 0)
    bson_append_document(&request, "$inc", 4, &inc);


  // Timestamps
  if (lastNotificationTime > 0) bson_append_double(&max, "lastNotification", 16, lastNotificationTime);
  if (lastSuccess          > 0) bson_append_double(&max, "lastSuccess",      11, lastSuccess);
  if (lastFailure          > 0) bson_append_double(&max, "lastFailure",      11, lastFailure);


  LM_T(LmtNotificationStats, ("%s: lastNotificationTime: %f", subscriptionId, lastNotificationTime));
  LM_T(LmtNotificationStats, ("%s: lastSuccess:          %f", subscriptionId, lastSuccess));
  LM_T(LmtNotificationStats, ("%s: lastFailure:          %f", subscriptionId, lastFailure));
  LM_T(LmtNotificationStats, ("%s: deltaAttempts:        %d", subscriptionId, deltaAttempts));
  LM_T(LmtNotificationStats, ("%s: deltaFailures:        %d", subscriptionId, deltaFailures));
  LM_T(LmtNotificationStats, ("%s: deltaNoMatch:         %d", subscriptionId, deltaNoMatch));
  LM_T(LmtNotificationStats, ("%s: forcedToPause:        %s", subscriptionId, (forcedToPause == true)? "TRUE" : "FALSE"));

  if (forcedToPause == true)
  {
    bson_t status;
    bson_init(&status);
    bson_append_utf8(&status, "status", 6, "inactive", 8);
    bson_append_document(&request, "$set", 4, &status);
    bson_destroy(&status);
  }

  bson_append_document(&request, "$max", 4, &max);

  MONGOC_WLOG("Updating Sub-Counters", tenantP->mongoDbName, "csubs", &selector, &request, LmtMongoc);
  bson_error_t  bError;
  bool          b = mongoc_collection_update_one(subscriptionsP, &selector, &request, NULL, &reply, &bError);

  if (b == false)
    LM_E(("mongoc error updating subscription counters/timestamps for '%s': [%d.%d]: %s", subscriptionId, bError.domain, bError.code, bError.message));
  else
    LM_T(LmtNotificationStats, ("%s: Successfully updated sub-counters/timestamps", subscriptionId));

  mongoc_client_pool_push(mongocPool, connectionP);
  mongoc_collection_destroy(subscriptionsP);

  bson_destroy(&reply);
  bson_destroy(&selector);
  bson_destroy(&inc);
  bson_destroy(&max);
  bson_destroy(&request);
}
