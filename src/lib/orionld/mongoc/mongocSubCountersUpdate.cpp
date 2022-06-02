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
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
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
void mongocSubCountersUpdate
(
  CachedSubscription*  cSubP,
  int                  deltaCount,
  double               lastNotificationTime,
  double               lastFailure,
  double               lastSuccess,
  bool                 ngsild
)
{
  LM_TMP(("SC: Updating subscription '%s'", cSubP->subscriptionId));
  LM_TMP(("SC: Count:                 %d", deltaCount));
  LM_TMP(("SC: lastNotificationTime:  %f", lastNotificationTime));
  LM_TMP(("SC: lastSuccess:           %f", lastSuccess));
  LM_TMP(("SC: lastFailure:           %f", lastFailure));

  mongocConnectionGet();

  if (orionldState.mongoc.subscriptionsP == NULL)
    orionldState.mongoc.subscriptionsP = mongoc_client_get_collection(orionldState.mongoc.client, orionldState.tenantP->mongoDbName, "csubs");

  bson_t  request;    // Entire request with count and timestamps to be updated
  bson_t  reply;
  bson_t  count;
  bson_t  max;
  bson_t  selector;

  bson_init(&reply);
  bson_init(&selector);
  bson_init(&count);
  bson_init(&max);
  bson_init(&request);

  // Selector - The _id is an OID if not NGSI-LD
  if (cSubP->ldContext != "")
    bson_append_utf8(&selector, "_id", 3, cSubP->subscriptionId, -1);
  else
  {
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, cSubP->subscriptionId);
    bson_append_oid(&selector, "_id", 3, &oid);
  }

  // Count
  if (deltaCount > 0)
    bson_append_int64(&count, "count", 5, deltaCount);

  // Timestamps
  if (lastNotificationTime > 0) bson_append_double(&max, "lastNotification", 16, lastNotificationTime);
  if (lastSuccess          > 0) bson_append_double(&max, "lastSuccess",      11, lastSuccess);
  if (lastFailure          > 0) bson_append_double(&max, "lastFailure",      11, lastFailure);

  if (deltaCount > 0)
    bson_append_document(&request, "$inc", 4, &count);
  bson_append_document(&request, "$max", 4, &max);

  LM_TMP(("Sending count+timestamps"));
  bool b = mongoc_collection_update_one(orionldState.mongoc.subscriptionsP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);
  LM_TMP(("Sent count+timestamps"));
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("SC: mongoc error updating subscription counters/timestamps for '%s': [%d.%d]: %s", cSubP->subscriptionId, errP->domain, errP->code, errP->message));
  }
  else
    LM_TMP(("SC: Successfully updated subscription '%s'", cSubP->subscriptionId));

  bson_destroy(&reply);
  bson_destroy(&selector);
  bson_destroy(&count);
  bson_destroy(&max);
  bson_destroy(&request);
}
