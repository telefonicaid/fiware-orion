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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/subCache.h"                                      // CachedSubscription, subCacheHeadGet, subCacheItemLookup

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/legacyDriver/legacyGetSubscriptions.h"         // legacyGetSubscriptions
#include "orionld/kjTree/kjTreeFromCachedSubscription.h"         // kjTreeFromCachedSubscription
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// tenantMatch -
//
static bool tenantMatch(OrionldTenant* requestTenantP, const char* subscriptionTenant)
{
  if (requestTenantP == &tenant0)
  {
    if (subscriptionTenant == NULL)
      return true;
  }
  else
  {
    if ((subscriptionTenant != NULL) && (strcmp(requestTenantP->tenant, subscriptionTenant) == 0))
      return true;
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// orionldGetSubscriptions -
//
bool orionldGetSubscriptions(void)
{
  if (experimental == false)
    return legacyGetSubscriptions();

  if (orionldState.uriParamOptions.fromDb == true)
  {
    //
    // GET Subscriptions with mongoc is yet to be implemented, so, we'll have to use the old Legacy function ...
    // BUT, not if mongocOnly is set
    //
    if (mongocOnly == true)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "this request does not support the new mongoc driver", 501);
      return false;
    }

    return legacyGetSubscriptions();
  }

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
      if (tenantMatch(orionldState.tenantP, cSubP->tenant) == true)
        ++count;
    }

    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);
  }

  if (limit != 0)
  {
    for (CachedSubscription* cSubP = subCacheHeadGet(); cSubP != NULL; cSubP = cSubP->next)
    {
      if (tenantMatch(orionldState.tenantP, cSubP->tenant) == false)
        continue;

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
