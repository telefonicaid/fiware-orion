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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/regMatchInformationItem.h"          // regMatchInformationItem



// -----------------------------------------------------------------------------
//
// regMatchInformationArray -
//
DistOp* regMatchInformationArray(RegCacheItem* regP, const char* entityId, const char* entityType, KjNode* incomingP)
{
  KjNode* informationV = kjLookup(regP->regTree, "information");

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    KjNode* attrUnion = regMatchInformationItem(regP, infoP, entityId, entityType, incomingP);

    if (attrUnion == NULL)
      continue;

    // If we get this far, then it's a match
    KjNode* entityIdP   = kjString(orionldState.kjsonP, "id",   entityId);
    KjNode* entityTypeP = kjString(orionldState.kjsonP, "type", entityType);

    kjChildAdd(attrUnion, entityIdP);
    kjChildAdd(attrUnion, entityTypeP);

    DistOp* distOpP = (DistOp*) kaAlloc(&orionldState.kalloc, sizeof(DistOp));

    distOpP->regP = regP;
    distOpP->body = attrUnion;

    return distOpP;
  }

  return NULL;
}
