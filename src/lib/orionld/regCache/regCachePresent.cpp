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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_T

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/common/tenantList.h"                           // tenant0



// -----------------------------------------------------------------------------
//
// regCachePresent -
//
void regCachePresent(void)
{
  for (OrionldTenant* tenantP = &tenant0; tenantP != NULL; tenantP = tenantP->next)
  {
    if (tenantP->regCache == NULL)
      LM_T(LmtRegCache, ("Tenant '%s': No regCache", tenantP->mongoDbName));
    else
    {
      LM_T(LmtRegCache, ("Tenant '%s':", tenantP->mongoDbName));
      RegCacheItem* rciP = tenantP->regCache->regList;

      while (rciP != NULL)
      {
        KjNode* regIdP = kjLookup(rciP->regTree, "id");

        LM_T(LmtRegCache, ("  o Registration %s:", (regIdP != NULL)? regIdP->value.s : "unknown"));
        LM_T(LmtRegCache, ("    o mode:  %s", registrationModeToString(rciP->mode)));
        LM_T(LmtRegCache, ("    o ops:   0x%x", rciP->opMask));

        if (rciP->idPatternRegexList != NULL)
        {
          LM_T(LmtRegCache, ("    o patterns:"));
          for (RegIdPattern* ripP = rciP->idPatternRegexList; ripP != NULL; ripP = ripP->next)
          {
            LM_T(LmtRegCache, ("      o %s (idPattern at %p)", ripP->owner->value.s, ripP->owner));
          }
        }
        else
          LM_T(LmtRegCache, ("    o patterns: NONE"));
        LM_T(LmtRegCache, ("  -----------------------------------"));
        rciP = rciP->next;
      }
    }
  }
}
