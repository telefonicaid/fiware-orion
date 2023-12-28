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
#include "kalloc/kaAlloc.h"                                       // kaAlloc
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjLookup.h"                                       // kjLookup
#include "kjson/kjBuilder.h"                                      // kjString, kjChildAdd
}

#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/types/StringArray.h"                            // StringArray
#include "orionld/regCache/RegCache.h"                            // RegCacheItem
#include "orionld/forwarding/DistOp.h"                            // DistOp
#include "orionld/forwarding/distOpListsMerge.h"                  // distOpListsMerge
#include "orionld/forwarding/regMatchInformationItemForQuery.h"   // regMatchInformationItemForQuery
#include "orionld/forwarding/regMatchInformationArrayForQuery.h"  // Own interface


// -----------------------------------------------------------------------------
//
// regMatchInformationArrayForQuery -
//
DistOp* regMatchInformationArrayForQuery(RegCacheItem* regP, StringArray* idListP, StringArray* typeListP, StringArray* attrListP)
{
  DistOp* distOpList   = NULL;
  KjNode* informationV = kjLookup(regP->regTree, "information");

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    DistOp* distOps = regMatchInformationItemForQuery(regP, infoP, idListP, typeListP, attrListP);

    //
    // regMatchEntityInfoForQuery fills in the entity id/type fields for the DistOps,
    // That is how we "respect" the registration
    // The search criteria is narrowed down using info from the registration
    //
    if (distOps != NULL)
      distOpList = distOpListsMerge(distOpList, distOps);
  }

  return distOpList;
}
