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
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray, kjChildAdd, ...
}

#include "orionld/common/orionldState.h"                         // orionldState, orionldEntityMap, orionldEntityMapId
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/kjTree/kjSort.h"                               // kjStringArraySort
#include "orionld/serviceRoutines/orionldGetEntityMap.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetEntityMap -
//
bool orionldGetEntityMap(void)
{
  const char* entityMapId = orionldState.wildcard[0];

  if (strcmp(entityMapId, orionldEntityMapId) != 0)  // For now we only have one single entity map
  {
    orionldError(OrionldResourceNotFound, "EntityMap Not Found", entityMapId, 404);
    return false;
  }

  // Cloning the entityMap - to later modify it
  orionldState.responseTree   = kjClone(orionldState.kjsonP, orionldEntityMap);

  // Sort all entity arrays in alphabetic order (for functests to work ...)
  // NOTE;
  //   Instead of doing that on entity map creation, it's done here, for performance reasons.
  //   It's a valid assumption that many more entity maps will be created than queried
  //
  for (KjNode* eV = orionldState.responseTree->value.firstChildP; eV != NULL; eV = eV->next)
  {
    kjStringArraySort(eV);
  }

  orionldState.httpStatusCode = 200;
  orionldState.noLinkHeader   = true;

  return true;
}
