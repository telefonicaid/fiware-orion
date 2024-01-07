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
#include "orionld/legacyDriver/kjTreeFromSubscription.h"         // kjTreeFromSubscription
#include "orionld/legacyDriver/legacyGetSubscription.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// legacyGetSubscription -
//
bool legacyGetSubscription(void)
{
  char*                 subscriptionId = orionldState.wildcard[0];
  CachedSubscription*   cSubP          = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);
  ngsiv2::Subscription  subscription;
  char*                 details = (char*) "subscription not found";

  subscription.descriptionProvided = false;
  subscription.expires             = -1;  // 0?
  subscription.throttling          = -1;  // 0?
  subscription.timeInterval        = -1;  // 0?

  if (mongoGetLdSubscription(&subscription, subscriptionId, orionldState.tenantP, &orionldState.httpStatusCode, &details) == false)
  {
    LM_E(("mongoGetLdSubscription error: %s", details));
    orionldError(OrionldResourceNotFound, details, subscriptionId, 404);
    return false;
  }

  // Transform to KjNode tree
  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = kjTreeFromSubscription(&subscription, cSubP, orionldState.contextP);

  return true;
}
