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

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/distOpCreate.h"                         // distOpCreate
#include "orionld/regMatch/regMatchInformationItem.h"            // regMatchInformationItem



// -----------------------------------------------------------------------------
//
// regMatchInformationArray -
//
DistOp* regMatchInformationArray
(
  RegCacheItem*  regP,
  DistOpType     operation,
  const char*    entityId,
  const char*    entityType,
  KjNode*        payloadBody
)
{
  DistOp* distOpList   = NULL;
  KjNode* informationV = kjLookup(regP->regTree, "information");

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    KjNode* attrUnion = regMatchInformationItem(regP, operation, infoP, entityId, entityType, payloadBody);

    if (attrUnion == NULL)
      continue;

    // If we get this far, then it's a match
    if (operation == DoCreateEntity)
    {
      if (kjLookup(attrUnion, "id") == NULL)
      {
        KjNode* entityIdP = kjString(orionldState.kjsonP, "id", entityId);
        kjChildAdd(attrUnion, entityIdP);
      }

      if (kjLookup(attrUnion, "type") == NULL)
      {
        KjNode* entityTypeP = kjString(orionldState.kjsonP, "type", entityType);
        kjChildAdd(attrUnion, entityTypeP);
      }
    }

    DistOp* distOpP = distOpCreate(operation, regP, NULL, NULL, NULL);

    distOpP->requestBody = attrUnion;

    if (distOpList == NULL)
      distOpList = distOpP;
    else
    {
      distOpP->next = distOpList;
      distOpList    = distOpP;
    }
  }

  return distOpList;
}
