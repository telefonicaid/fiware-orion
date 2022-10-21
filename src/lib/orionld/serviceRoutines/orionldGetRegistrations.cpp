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
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/legacyDriver/legacyGetRegistrations.h"         // legacyGetRegistrations
#include "orionld/mongoc/mongocRegistrationsGet.h"               // mongocRegistrationsGet
#include "orionld/dbModel/dbModelToApiRegistration.h"            // dbModelToApiRegistration
#include "orionld/regCache/RegCache.h"                           // RegCache, RegCacheItem
#include "orionld/regCache/regCacheGet.h"                        // regCacheGet
#include "orionld/serviceRoutines/orionldGetRegistrations.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// apiModelFromCachedRegistration - FIXME: Move to dbModel library?
//
extern void apiModelFromCachedRegistration(KjNode* regTree, RegCacheItem* cachedRegP, bool sysAttrs);



// ----------------------------------------------------------------------------
//
// orionldGetRegistrations -
//
bool orionldGetRegistrations(void)
{
  if (experimental == false)
    return legacyGetRegistrations();

  if (orionldState.uriParamOptions.fromDb == true)
  {
    //
    // GET Registrations with mongoc
    //
    // return orionldGetRegistrationsFromDb();
  }

  //
  // Not Legacy, not "From DB" - Getting the registrations from the registration cache
  //
  RegCache* rcP = regCacheGet(orionldState.tenantP, false);

  if (orionldState.uriParams.count == true)  // Empty loop over the registrations, just to count how many there are
  {
    int count  = 0;

    if (rcP != NULL)
    {
      for (RegCacheItem* cRegP = rcP->regList; cRegP != NULL; cRegP = cRegP->next)
      {
        ++count;
      }
    }

    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);
  }

  int        offset    = orionldState.uriParams.offset;
  int        limit     = orionldState.uriParams.limit;
  KjNode*    regArray  = kjArray(orionldState.kjsonP, NULL);
  int        regs      = 0;
  int        ix        = 0;

  if ((limit != 0) && (rcP != NULL))
  {
    for (RegCacheItem* cRegP = rcP->regList; cRegP != NULL; cRegP = cRegP->next)
    {
      if (ix < offset)
      {
        ++ix;
        continue;
      }

      KjNode* apiRegP = kjClone(orionldState.kjsonP, cRegP->regTree);  // Work on a cloned copy from the reg-cache
      apiModelFromCachedRegistration(apiRegP, cRegP, orionldState.uriParamOptions.sysAttrs);
      kjChildAdd(regArray, apiRegP);

      ++ix;
      ++regs;
      if (regs >= limit)
        break;
    }
  }
  else
    orionldState.noLinkHeader = true;  // Don't want the Link header if there is no payload body (empty array)

  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = regArray;

  return true;
}
