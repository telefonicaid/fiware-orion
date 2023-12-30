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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState, kjTreeLog
#include "orionld/distOp/distOpCreate.h"                         // distOpCreate
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpAttrs.h"                          // distOpAttrs
#include "orionld/regMatch/regMatchEntityInfoForQuery.h"         // regMatchEntityInfoForQuery
#include "orionld/regMatch/regMatchAttributesForGet.h"           // regMatchAttributesForGet
#include "orionld/regMatch/regMatchInformationItemForQuery.h"    // Own interface



// -----------------------------------------------------------------------------
//
// regMatchInformationItemForQuery -
//
DistOp* regMatchInformationItemForQuery
(
  RegCacheItem* regP,
  KjNode*       infoP,
  StringArray*  idListP,
  StringArray*  typeListP,
  StringArray*  attrListP
)
{
  KjNode* entities   = kjLookup(infoP, "entities");
  DistOp* distOpList = NULL;

  if (entities != NULL)
  {
    DistOp* distOpP = NULL;

    for (KjNode* entityInfoP = entities->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      //
      // Creating a DistOp, in case entityInfoP matches (regMatchEntityInfoForQuery fills it in on hit).
      // If used:
      //   - distOpP is inserted in distOpList,
      //   - it is NULLed, and
      //   - it is allocated again in the next round of the loop
      //
      distOpP = distOpCreate(DoQueryEntity, regP, idListP, typeListP, attrListP);

      if (regMatchEntityInfoForQuery(regP, entityInfoP, idListP, typeListP, distOpP) == true)
      {
        if (distOpList == NULL)
          distOpList = distOpP;
        else
        {
          distOpP->next = distOpList;
          distOpList    = distOpP;
        }
      }
    }

    if (distOpList == NULL)
      return NULL;
  }
  else
  {
    DistOp* distOpP = distOpCreate(DoQueryEntity, regP, NULL, typeListP, attrListP);

    if (distOpList == NULL)
      distOpList = distOpP;
    else
    {
      distOpP->next = distOpList;
      distOpList    = distOpP;
    }
  }

  //
  // The attributes are the same for all matching entity array items
  //
  KjNode*      propertyNamesP     = kjLookup(infoP, "propertyNames");
  KjNode*      relationshipNamesP = kjLookup(infoP, "relationshipNames");
  StringArray* attrUnionP         = regMatchAttributesForGet(regP, propertyNamesP, relationshipNamesP, attrListP, NULL);

  if (attrUnionP == NULL)
    return NULL;

  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    distOpP->attrList = attrUnionP;
    if ((distOpP->attrList != NULL) && (distOpP->attrList->items > 0))
      distOpAttrs(distOpP, distOpP->attrList);
    else
      distOpP->attrsParam = NULL;
  }

  return distOpList;
}
