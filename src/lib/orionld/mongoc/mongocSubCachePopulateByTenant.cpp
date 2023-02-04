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
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // mongocPool
#include "orionld/common/subCacheApiSubscriptionInsert.h"        // subCacheApiSubscriptionInsert
#include "orionld/dbModel/dbModelToApiSubscription.h"            // dbModelToApiSubscription
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/q/QNode.h"                                     // QNode
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocSubCachePopulateByTenant.h"       // Own interface



/* ****************************************************************************
*
* mongocSubCachePopulateByTenant -
*
* 2. Lookup all subscriptions in the database
* 3. Insert them again in the cache (with fresh data from database)
*
* NOTE
*   The query for the database ONLY extracts the interesting subscriptions:
*   - "conditions.type" << "ONCHANGE"
*
*   I.e. the subscriptions is for ONCHANGE.
*
* IMPORTANT NOTE:
*   As this function is called outside of the "Request Threads", orionldState cannot be used!
*/
bool mongocSubCachePopulateByTenant(OrionldTenant* tenantP)
{
  bson_t            mongoFilter;
  const bson_t*     mongoDocP;
  mongoc_cursor_t*  mongoCursorP;
  bson_error_t      mongoError;
  char*             title;
  char*             details;

  //
  // Empty filter for the query - we want ALL subscriptions
  //
  bson_init(&mongoFilter);

  mongoc_client_t* connectionP = mongoc_client_pool_pop(mongocPool);

  mongoc_collection_t* subscriptionsP = mongoc_client_get_collection(connectionP, tenantP->mongoDbName, "csubs");

  //
  // Run the query
  //
  // semTake(&mongoSubscriptionsSem);
  if ((mongoCursorP = mongoc_collection_find_with_opts(subscriptionsP, &mongoFilter, NULL, NULL)) == NULL)
  {
    mongoc_client_pool_push(mongocPool, connectionP);
    mongoc_collection_destroy(subscriptionsP);
    LM_RE(false, ("Internal Error (mongoc_collection_find_with_opts ERROR)"));
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    KjNode* dbSubP = mongocKjTreeFromBson(mongoDocP, &title, &details);

    if (dbSubP == NULL)
    {
      LM_E(("Database Error (unable to create tree of subscriptions for tenant '%s')", tenantP->tenant));
      continue;
    }

    QNode*  qTree        = NULL;
    KjNode* contextNodeP = NULL;
    KjNode* coordinatesP = NULL;
    KjNode* apiSubP      = dbModelToApiSubscription(dbSubP, tenantP->tenant, true, &qTree, &coordinatesP, &contextNodeP);

    if (apiSubP == NULL)
      continue;

    OrionldContext* contextP = NULL;
    if (contextNodeP != NULL)
      contextP = orionldContextFromUrl(contextNodeP->value.s, NULL);

    subCacheApiSubscriptionInsert(apiSubP, qTree, coordinatesP, contextP, tenantP->tenant);
  }

  mongoc_client_pool_push(mongocPool, connectionP);
  mongoc_collection_destroy(subscriptionsP);

  bool r = true;

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    r = false;
    LM_E(("Internal Error (DB Error '%s')", mongoError.message));
  }

  mongoc_cursor_destroy(mongoCursorP);
  bson_destroy(&mongoFilter);

  return r;
}
