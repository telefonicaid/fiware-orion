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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_WLOG
#include "orionld/mongoc/mongocSubscriptionInsert.h"             // Own interface



// -----------------------------------------------------------------------------
//
// mongocSubscriptionInsert -
//
// For now, only POST /subscriptions uses this function
//
bool mongocSubscriptionInsert(KjNode* dbSubscriptionP, const char* subscriptionId)
{
  bson_t document;
  bson_t reply;

  mongocConnectionGet(orionldState.tenantP, DbSubscriptions);

  bson_init(&document);
  bson_init(&reply);

  mongocKjTreeToBson(dbSubscriptionP, &document);

  MONGOC_WLOG("Inserting a Subscription", orionldState.tenantP->mongoDbName, "csubs", NULL, &document, LmtMongoc);
  bool b = mongoc_collection_insert_one(orionldState.mongoc.subscriptionsP, &document, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    char          eBuf[1024];
    bson_error_t* errP = &orionldState.mongoc.error;

    snprintf(eBuf, sizeof(eBuf), "mongoc error inserting subscription '%s': [%d.%d]: %s", subscriptionId, errP->domain, errP->code, errP->message);
    LM_E(("%s", eBuf));
    orionldError(OrionldInternalError, "Database Error", eBuf, 500);
  }

  // mongocConnectionRelease(); - done at the end of the request

  bson_destroy(&document);
  bson_destroy(&reply);

  return b;
}
