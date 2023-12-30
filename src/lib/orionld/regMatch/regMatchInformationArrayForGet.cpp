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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjChildAdd
}

#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/distOpCreate.h"                         // distOpCreate
#include "orionld/distOp/distOpAttrs.h"                          // distOpAttrs
#include "orionld/regMatch/regMatchInformationItemForGet.h"      // Own interface



// -----------------------------------------------------------------------------
//
// regMatchInformationArrayForGet -
//
DistOp* regMatchInformationArrayForGet(RegCacheItem* regP, const char* entityId, const char* entityType, StringArray* attrV, const char* geoProp)
{
  KjNode* informationV = kjLookup(regP->regTree, "information");

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    StringArray* attrList = regMatchInformationItemForGet(regP, infoP, entityId, entityType, attrV, geoProp);

    if (attrList == NULL)  // No match
      continue;

    // If we get this far, then it's a match and we can create the DistOp item and return
    DistOp* distOpP = distOpCreate(DoQueryEntity, regP, NULL, NULL, attrList);

    if ((distOpP->attrList != NULL) && (distOpP->attrList->items > 0))
      distOpAttrs(distOpP, distOpP->attrList);

    distOpP->geometryProperty  = (char*) geoProp;

    return distOpP;
  }

  return NULL;
}
