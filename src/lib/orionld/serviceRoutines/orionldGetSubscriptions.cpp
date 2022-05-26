/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include <vector>

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "common/string.h"                                       // toString
#include "rest/uriParamNames.h"                                  // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
#include "cache/subCache.h"                                      // CachedSubscription, subCacheHeadGet, subCacheItemLookup
#include "mongoBackend/mongoGetSubscriptions.h"                  // mongoListSubscriptions

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/kjTree/kjTreeFromCachedSubscription.h"         // kjTreeFromCachedSubscription
#include "orionld/kjTree/kjTreeFromSubscription.h"               // kjTreeFromSubscription
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetSubscriptionsWithMongoBackend -
//
static bool orionldGetSubscriptionsWithMongoBackend(void)
{
  std::vector<ngsiv2::Subscription> subVec;
  OrionError                        oe;
  int64_t                           count  = 0;

  mongoGetLdSubscriptions("/#", &subVec, orionldState.tenantP, (long long*) &count, &oe);

  if (orionldState.uriParams.count == true)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  for (unsigned int ix = 0; ix < subVec.size(); ix++)
  {
    CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subVec[ix].id.c_str());

    if (cSubP != NULL)
    {
      KjNode* subscriptionNodeP = kjTreeFromSubscription(&subVec[ix], cSubP, orionldState.contextP);
      kjChildAdd(orionldState.responseTree, subscriptionNodeP);
    }
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldGetSubscriptions -
//
bool orionldGetSubscriptions(void)
{
  if ((experimental == false) || (orionldState.uriParamOptions.fromDb == true))
    return orionldGetSubscriptionsWithMongoBackend();

  int      offset    = orionldState.uriParams.offset;
  int      limit     = orionldState.uriParams.limit;
  KjNode*  subArray  = kjArray(orionldState.kjsonP, NULL);
  int      ix        = 0;
  int      subs      = 0;

  if (orionldState.uriParams.count == true)  // Empty loop over the subscriptions, just to count how many there are
  {
    int count = 0;
    for (CachedSubscription* cSubP = subCacheHeadGet(); cSubP != NULL; cSubP = cSubP->next)
    {
      ++count;
    }
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);
  }

  if (limit != 0)
  {
    for (CachedSubscription* cSubP = subCacheHeadGet(); cSubP != NULL; cSubP = cSubP->next)
    {
      if (ix < offset)
      {
        ++ix;
        continue;
      }

      KjNode* subP = kjTreeFromCachedSubscription(cSubP, orionldState.uriParamOptions.sysAttrs, orionldState.out.contentType == JSONLD);

      if (subP == NULL)
      {
        LM_E(("Internal Error (kjTreeFromCachedSubscription failed for subscription '%s')", cSubP->subscriptionId));
        ++ix;
        continue;
      }

      kjChildAdd(subArray, subP);
      ++ix;
      ++subs;

      if (subs >= limit)
        break;
    }
  }
  else
    orionldState.noLinkHeader = true;  // Don't want the Link header if there is no payload body (empty array)

  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = subArray;

  return true;
}
