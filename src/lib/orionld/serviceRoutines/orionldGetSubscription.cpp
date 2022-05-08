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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/mongoGetSubscriptions.h"                  // mongoGetLdSubscription
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "cache/subCache.h"                                      // CachedSubscription, subCacheItemLookup
#include "orionld/kjTree/kjTreeFromSubscription.h"               // kjTreeFromSubscription
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/serviceRoutines/orionldGetSubscription.h"      // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetSubscription -
//
bool orionldGetSubscription(void)
{
  char*                 subscriptionId = orionldState.wildcard[0];
  ngsiv2::Subscription  subscription;
  char*                 details = (char*) "subscription not found";
  CachedSubscription*   cSubP   = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);

#if 0
  LM_TMP(("Looking up sub '%s' in the sub-cache (tenant: '%s')", subscriptionId, orionldState.tenantP->tenant));
  if (cSubP != NULL)
  {
    LM_TMP(("Found it in the cache - need to render a KjNode tree from the cache contents ..."));
    // orionldState.httpStatusCode = SccOk;
    // orionldState.responseTree   = kjTreeFromCachedSubscription(cSubP);
    // kjTreeLog(orionldState.responseTree, "Subscription");

    // return true;
  }
#endif

  subscription.descriptionProvided = false;
  subscription.expires             = -1;  // 0?
  subscription.throttling          = -1;  // 0?
  subscription.timeInterval        = -1;  // 0?

  // if (cSubP == NULL) - once I have the function to "render a KjNode tree from the cache contents"
  {
    if (mongoGetLdSubscription(&subscription, subscriptionId, orionldState.tenantP, &orionldState.httpStatusCode, &details) == false)
    {
      LM_E(("mongoGetLdSubscription error: %s", details));
      orionldError(OrionldResourceNotFound, details, subscriptionId, 404);
      return false;
    }
  }

  // Transform to KjNode tree
  orionldState.httpStatusCode = SccOk;
  orionldState.responseTree   = kjTreeFromSubscription(&subscription, cSubP);

  return true;
}
