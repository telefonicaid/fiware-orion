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
#include "orionld/forwarding/distOpListDebug.h"                   // distOpListDebug
#include "orionld/forwarding/regMatchInformationItemForQuery.h"   // regMatchInformationItemForQuery
#include "orionld/forwarding/regMatchInformationArrayForQuery.h"  // Own interface


#if 0
// -----------------------------------------------------------------------------
//
// distOpEnqueueOne -
//
DistOp* distOpEnqueueOne(DistOp* listP, DistOp* newItemP)
{
  if (listP != NULL)
    newItemP->next = listP;  // Enqueue in the beginning of the list

  return newItemP;
}



// -----------------------------------------------------------------------------
//
// distOpEnqueueMany -
//
DistOp* distOpEnqueueMany(DistOp* listP, DistOp* newItemP)
{
  if (listP == NULL)
    return newItemP;

  // Find last item in listP
  DistOp* lastItemP = listP;
  while (lastItemP->next != NULL)
    lastItemP = lastItemP->next;

  lastItemP->next = listP;

  return listP;
}
#endif



// -----------------------------------------------------------------------------
//
// regMatchInformationArrayForQuery -
//
DistOp* regMatchInformationArrayForQuery(RegCacheItem* regP, StringArray* idListP, StringArray* typeListP, StringArray* attrListP)
{
  DistOp* distOpList   = NULL;
  KjNode* informationV = kjLookup(regP->regTree, "information");
  int     infos        = 0;

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    KjNode*      entitiesItemP = NULL;
    StringArray* attrList      = regMatchInformationItemForQuery(regP, infoP, idListP, typeListP, attrListP, &entitiesItemP);

    if (attrList == NULL)  // No match
      continue;

    // If we get this far, then it's a match and we can create the DistOp item and return
    DistOp* distOpP = (DistOp*) kaAlloc(&orionldState.kalloc, sizeof(DistOp));

    distOpP->regP       = regP;
    distOpP->operation  = DoQueryEntity;
    distOpP->attrList   = attrList;

    //
    // To fill in 'idList' and 'typeList' of 'distOpP', we need the matching item in "entities"
    //
    if (entitiesItemP != NULL)
    {
      //
      // This is how we "respect" the registration
      // The search criteria is narrowed down usinf info from the registration
      //
      distOpP->idList   = NULL;
      distOpP->typeList = NULL;

      KjNode* entityIdP        = kjLookup(entitiesItemP, "id");
      KjNode* entityIdPatternP = kjLookup(entitiesItemP, "idPattern");
      KjNode* entityTypeP      = kjLookup(entitiesItemP, "type");

      if (entityIdP != NULL)
        distOpP->entityId = entityIdP->value.s;
      else if (entityIdPatternP != NULL)
        distOpP->entityIdPattern = entityIdPatternP->value.s;

      if (entityTypeP != NULL)  // "type" is mandatory. entityTypeP cannot be NULL
        distOpP->entityType = entityTypeP->value.s;
    }
    else
    {
      distOpP->idList     = idListP;
      distOpP->typeList   = typeListP;
    }

    distOpList = distOpListsMerge(distOpList, distOpP);
    ++infos;
  }

  LM(("%s: %d matching infos", regP->regId, infos));

  distOpListDebug(distOpList, "partial list, end of regMatchInformationArrayForQuery");
  return distOpList;
}
