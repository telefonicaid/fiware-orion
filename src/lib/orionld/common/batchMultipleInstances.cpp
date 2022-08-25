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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/common/batchMultipleInstances.h"               // Own interface



// ----------------------------------------------------------------------------
//
// batchMultipleInstances -
//
// IMPORTANT NOTE
//   The entity type cannot be modified (well, not until multi-typing is implemented).
//   The "entity-type-change check" is already implemented - for entities that existed in the DB prior to the batch upsert request.
//   BUT, what if the entity does not exist, but we have more than one instance of an entity ?
//
//   Well, in such case, the first instance creates the entity (AND DEFINES THE ENTITY TYPE).
//   Instances after this first ("creating") instance MUST have the same entity type. If not, they're erroneous.
//   - Erroneous instances must be removed also for TRoE
//
bool batchMultipleInstances(const char* entityId, KjNode* updatedArrayP, KjNode* createdArrayP)
{
  for (KjNode* itemP = updatedArrayP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(entityId, itemP->value.s) == 0)
      return true;
  }

  if (createdArrayP != NULL)
  {
    for (KjNode* itemP = createdArrayP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      if (strcmp(entityId, itemP->value.s) == 0)
        return true;
    }
  }

  return false;
}
