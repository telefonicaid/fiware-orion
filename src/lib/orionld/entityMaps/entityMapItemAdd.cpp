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
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjBuilder.h"                                        // kjChildAdd
}

#include "logMsg/logMsg.h"                                              // LM_*

#include "orionld/types/EntityMap.h"                                    // EntityMap
#include "orionld/types/DistOp.h"                                       // DistOp
#include "orionld/entityMaps/entityMapItemAdd.h"                        // Own interface



// -----------------------------------------------------------------------------
//
// entityMapItemAdd -
//
void entityMapItemAdd(EntityMap* entityMap, const char* entityId, DistOp* distOpP)
{
  KjNode* matchP = kjLookup(entityMap->map, entityId);

  LM_T(LmtCount, ("entity id: '%s'", entityId));

  if (matchP == NULL)
  {
    //
    // The entity ID is not present in the list - must be added
    //
    LM_T(LmtEntityMap, ("The entity ID '%s' is not present in the list - adding it", entityId));
    matchP = kjArray(NULL, entityId);
    kjChildAddSorted(entityMap->map, matchP);
  }

  //
  // Add DistOp ID to matchP's array - remember it's global, can't use kaAlloc
  //
  const char*  distOpId      = (distOpP != NULL)? distOpP->regP->regId : "@none";
  KjNode*      distOpIdNodeP = kjString(NULL, NULL, distOpId);

  LM_T(LmtEntityMap, ("Adding DistOp '%s' to entity '%s'", distOpId, matchP->name));
  kjChildAdd(matchP, distOpIdNodeP);  // This is sorted later - in entityMapCreate
}
