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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include "logMsg/logMsg.h"                             // LM_*
#include "logMsg/traceLevels.h"                        // Lmt*

#include "cache/subCache.h"                            // CachedSubscription, subCacheItemLookup, ...

#include "orionld/common/orionldState.h"               // orionldState
#include "orionld/common/orionldError.h"               // orionldError
#include "orionld/payloadCheck/PCHECK.h"               // PCHECK_URI
#include "orionld/mongoCppLegacy/mongoCppLegacySubscriptionGet.h"      // mongoCppLegacySubscriptionGet
#include "orionld/mongoCppLegacy/mongoCppLegacySubscriptionDelete.h"   // mongoCppLegacySubscriptionDelete
#include "orionld/legacyDriver/legacyDeleteSubscription.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// legacyDeleteSubscription -
//
bool legacyDeleteSubscription(void)
{
  PCHECK_URI(orionldState.wildcard[0], true, 0, "Invalid Subscription Identifier", orionldState.wildcard[0], 400);

  if (mongoCppLegacySubscriptionGet(orionldState.wildcard[0]) == NULL)
  {
    orionldError(OrionldResourceNotFound, "Subscription not found", orionldState.wildcard[0], 404);
    return false;
  }

  if (mongoCppLegacySubscriptionDelete(orionldState.wildcard[0]) == false)
  {
    orionldError(OrionldResourceNotFound, "Subscription not found", orionldState.wildcard[0], 404);
    return false;
  }

  if (noCache == false)
  {
    CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, orionldState.wildcard[0]);
    if (cSubP != NULL)
      subCacheItemRemove(cSubP);
    else
      LM_W(("The subscription '%s' was successfully removed from DB but does not exist in sub-cache ... (sub-cache is enabled)"));
  }

  orionldState.httpStatusCode = 204;

  return true;
}
