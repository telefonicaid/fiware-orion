/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/SubCache.h"                              // SubCache
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/mongoc/mongocSubscriptionsIter.h"              // mongocSubscriptionsIter
#include "orionld/dbModel/dbModelToApiSubscription.h"            // dbModelToApiSubscription
#include "orionld/subCache/subCacheItemAdd.h"                    // subCacheItemAdd
#include "orionld/subCache/subCacheCreate.h"                     // Own interface



extern void apiModelToCacheSubscription(KjNode* apiSubscriptionP);
// -----------------------------------------------------------------------------
//
// subIterFunc -
//
int subIterFunc(SubCache* scP, KjNode* dbSubP)
{
  // Convert DB Sub to API Sub
  if (dbModelToApiSubscription(dbSubP, true, true) == false)
    LM_RE(-1, ("dbModelToApiSubscription failed"));

  // The DB Subscription 'dbSubP' is now in API Subscription format (after calling dbModelToApiSubscription)
  KjNode* apiSubP = dbSubP;

  // If a jsonldContext is given for the subscription, make sure it's valid
  OrionldContext* jsonldContextP = NULL;
  KjNode*         jsonldContextNodeP = kjLookup(apiSubscriptionP, "jsonldContext");
  if (jsonldContextNodeP != NULL)
  {
    jsonldContextP = orionldContextFromUrl(jsonldContextNodeP->value.s, NULL);

    if (jsonldContextP == NULL)
    {
      LM_W(("Unable to resolve a Subscription @context for a sub-cache item"));
      return 0;
    }
  }

  // Subscription Id
  KjNode* subIdNodeP = kjLookup(apiSubP, "id");
  char*   subId      = (subIdNodeP != NULL)? subIdNodeP->value.s : (char*) "no:sub:id";

  // Convert API Sub to Cache Sub
  apiModelToCacheSubscription(apiSubP);

  // Insert cacheSubP in tenantP->subCache
  subCacheItemAdd(scP, subId, apiSubP, true, jsonldContextP);

  return 0;
}



// -----------------------------------------------------------------------------
//
// subCacheCreate -
//
SubCache* subCacheCreate(OrionldTenant* tenantP, bool scanSubs)
{
  SubCache* scP = (SubCache*) malloc(sizeof(SubCache));

  if (scP == NULL)
    LM_RE(NULL, ("Out of memory (attempt to create a subscription cache)"));

  scP->tenantP  = tenantP;
  scP->subList  = NULL;
  scP->last     = NULL;

  if (scanSubs)
  {
    if (mongocSubscriptionsIter(scP, subIterFunc) != 0)
      LM_E(("mongocSubscriptionsIter failed"));
  }

  return scP;
}
