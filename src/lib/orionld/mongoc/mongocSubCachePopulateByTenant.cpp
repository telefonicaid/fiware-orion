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

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/common/orionldState.h"                         // mongocPool
#include "orionld/common/subCacheApiSubscriptionInsert.h"        // subCacheApiSubscriptionInsert
#include "orionld/pernot/pernotSubCacheAdd.h"                    // pernotSubCacheAdd
#include "orionld/dbModel/dbModelToApiSubscription.h"            // dbModelToApiSubscription
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_RLOG
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
bool mongocSubCachePopulateByTenant(OrionldTenant* tenantP, bool refresh)
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

  mongoc_client_t*     connectionP    = mongoc_client_pool_pop(mongocPool);
  mongoc_collection_t* subscriptionsP = mongoc_client_get_collection(connectionP, tenantP->mongoDbName, "csubs");

  if (subscriptionsP == NULL)
    LM_X(1, ("mongoc_client_get_collection failed for 'csubs' collection on tenant '%s'", tenantP->mongoDbName));

  //
  // Run the query
  //
  // semTake(&mongoSubscriptionsSem);
  MONGOC_RLOG("Subscription for sub-cache", tenantP->mongoDbName, "subscriptions", &mongoFilter, NULL, LmtMongoc);
  if ((mongoCursorP = mongoc_collection_find_with_opts(subscriptionsP, &mongoFilter, NULL, NULL)) == NULL)
  {
    mongoc_client_pool_push(mongocPool, connectionP);
    mongoc_collection_destroy(subscriptionsP);
    LM_RE(false, ("Internal Error (mongoc_collection_find_with_opts ERROR)"));
  }

  if (refresh == true)
  {
    //
    // Algorithm for "Delete subs that are no longer in DB":
    //        1. Before the loop: Mark all subs in cache as "not in DB"
    //        2. Inside the loop: When a sub is found in DB, mark it as "in DB"
    //        3. After the loop:  Lookup all cached subs and remove thos "not in DB"
    //

    // Loop over the entire sub-cache and mark all subscriptions with inDB == false
    char* tenantName = (tenantP != NULL)? tenantP->tenant : NULL;
    for (CachedSubscription* cSubP = subCacheHeadGet(); cSubP != NULL; cSubP = cSubP->next)
    {
      LM_T(LmtSubCacheSync, ("tenantName: '%s' (cSubP->tenant: '%s')", tenantName, cSubP->tenant));
      if (tenantMatch(tenantName, cSubP->tenant) == true)
        cSubP->inDB = false;
    }
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    KjNode* dbSubP = mongocKjTreeFromBson(mongoDocP, &title, &details);

    if (dbSubP == NULL)
    {
      LM_E(("Database Error (unable to create tree of subscriptions for tenant '%s')", tenantP->tenant));
      continue;
    }

    QNode*              qTree         = NULL;
    KjNode*             contextNodeP  = NULL;
    KjNode*             coordinatesP  = NULL;
    KjNode*             showChangesP  = NULL;
    KjNode*             sysAttrsP     = NULL;
    OrionldRenderFormat renderFormat  = RF_NORMALIZED;
    double              timeInterval  = 0;

    kjTreeLog(dbSubP, "dbSubP", LmtPernot);
    KjNode*      apiSubP       = dbModelToApiSubscription(dbSubP,
                                                          tenantP->tenant,
                                                          true,
                                                          &qTree,
                                                          &coordinatesP,
                                                          &contextNodeP,
                                                          &showChangesP,
                                                          &sysAttrsP,
                                                          &renderFormat,
                                                          &timeInterval);

    if (apiSubP == NULL)
      continue;

    kjTreeLog(apiSubP, "apiSubP", LmtPernot);

    OrionldContext* contextP = NULL;
    if (contextNodeP != NULL)
      contextP = orionldContextFromUrl(contextNodeP->value.s, NULL);

    if (timeInterval == 0)
    {
      CachedSubscription* cSubP = subCacheApiSubscriptionInsert(apiSubP, qTree, coordinatesP, contextP, tenantP->tenant, showChangesP, sysAttrsP, renderFormat);
      cSubP->inDB = true;
    }
    else
      pernotSubCacheAdd(NULL, apiSubP, NULL, qTree, coordinatesP, contextP, tenantP, showChangesP, sysAttrsP, renderFormat, timeInterval);
  }

  if (refresh == true)
  {
    // Now loop over the entire sub-cache and remove those subscriptions with inDB == false;
    CachedSubscription* cSubP = subCacheHeadGet();
    CachedSubscription* next;

    char* tenantName = (tenantP != NULL)? tenantP->tenant : NULL;
    while (cSubP != NULL)
    {
      next = cSubP->next;

      if ((cSubP->inDB == false) && (tenantMatch(tenantName, cSubP->tenant) == true))
      {
        subCacheItemRemove(cSubP);
      }

      cSubP = next;
    }
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
