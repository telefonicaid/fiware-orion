/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kalloc/kaAlloc.h"                                         // kaAlloc
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/types/DistOpListItem.h"                           // DistOpListItem
#include "orionld/common/orionldState.h"                            // orionldState, entityMaps
#include "orionld/regCache/regCacheItemLookup.h"                    // regCacheItemLookup
#include "orionld/distOp/distOpCreate.h"                            // distOpCreate
#include "orionld/distOp/distOpLookupByRegId.h"                     // distOpLookupByRegId



// -----------------------------------------------------------------------------
//
// distOpListItemCreate -
//
DistOpListItem* distOpListItemCreate(const char* distOpId, char* idString)
{
  LM_T(LmtDistOpList, ("orionldState.distOpList at %p", orionldState.distOpList));
  DistOp* distOpP = distOpLookupByRegId(orionldState.distOpList, distOpId);
  LM_T(LmtDistOpList, ("Response: %p", distOpP));

  if (distOpP == NULL)
  {
#if 0
    //
    // I think this LM_RE here is incorrect.
    // The DistOps are for one request only.
    // So, create a new DistOp if it does not exist.
    //
    LM_RE(NULL, ("Internal Error (unable to find the DistOp '%s'", distOpId));
#else
    RegCacheItem* rciP = regCacheItemLookup(orionldState.tenantP->regCache, distOpId);
    distOpP = distOpCreate(DoQueryEntity, rciP, NULL, NULL, NULL);
    // Add DistOp to the linked list of DistOps
    distOpP->next    = orionldState.distOpList;
    orionldState.distOpList = distOpP;
#endif
  }

  DistOpListItem* itemP = (DistOpListItem*) kaAlloc(&orionldState.kalloc, sizeof(DistOpListItem));
  if (itemP == NULL)
    LM_X(1, ("Out of memory"));

  itemP->distOpP   =  distOpP;
  itemP->next      = NULL;
  itemP->entityIds = idString;

  return itemP;
}

