/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kbase/kMacros.h"                                     // K_VEC_SIZE
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/troeIgnored.h"                        // troeIgnored
#include "orionld/common/duplicatedInstances.h"                // Own interface



// -----------------------------------------------------------------------------
//
// troeIgnoreMark -
//
static void troeIgnoreMark(KjNode* entityP)
{
  if (orionldState.troeIgnoreIx >= K_VEC_SIZE(orionldState.troeIgnoreV))
  {
    LM_W(("TRoE ignore index overflow - this adds an extra entity-instance to the history - should not change anything"));
    return;
  }

  orionldState.troeIgnoreV[orionldState.troeIgnoreIx++] = entityP;
}



// -----------------------------------------------------------------------------
//
// entityIdLookup -
//
static KjNode* entityIdLookup(KjNode* array, const char* entityId)
{
  for (KjNode* nodeP = array->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    KjNode* idP = kjLookup(nodeP, "id");

    if (idP == NULL)
      continue;  // It's an error, but it should never happen

    if (strcmp(idP->value.s, entityId) == 0)
      return nodeP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// entityInstanceLookup -
//
static KjNode* entityInstanceLookup(KjNode* array, const char* entityId, KjNode* entityP)
{
  for (KjNode* nodeP = array->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (nodeP == entityP)
      continue;

    KjNode* idP = kjLookup(nodeP, "id");
    if (idP == NULL)
      continue;  // It's an error, but it should never happen

    if (strcmp(idP->value.s, entityId) == 0)
      return nodeP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// kjEntityMergeReplacingAttributes - merge 'copyP' into 'entityP', replacing attributes
//
static void kjEntityMergeReplacingAttributes(KjNode* entityP, KjNode* copyP)
{
  KjNode* next;
  KjNode* attrP = copyP->value.firstChildP;

  while (attrP != NULL)
  {
    if (attrP->type != KjObject)
    {
      attrP = attrP->next;
      continue;
    }

    next = copyP->next;

    //
    // Got an attribute - if found in 'entityP' then remove it from there
    // then move the attribute from 'copyP' to 'entityP'
    //
    // => REPLACE the attr in 'entityP' with the one from 'copyP'
    //
    KjNode* toRemove = kjLookup(entityP, attrP->name);

    if (toRemove != NULL)
      kjChildRemove(entityP, toRemove);

    kjChildRemove(copyP, attrP);
    kjChildAdd(entityP, attrP);
    attrP = next;
  }
}



// -----------------------------------------------------------------------------
//
// kjEntityMergeIgnoringExistingAttributes - merge 'copyP' into 'entityP', ignoring already existing attributes
//
static void kjEntityMergeIgnoringExistingAttributes(KjNode* entityP, char* entityId, KjNode* dbEntityV, KjNode* copyP)
{
  KjNode* next;
  KjNode* attrP = copyP->value.firstChildP;

  while (attrP != NULL)
  {
    if (attrP->type != KjObject)
    {
      attrP = attrP->next;
      continue;
    }

    next = copyP->next;

    //
    // Got an attribute - if found in 'DB entity' OR in previous instance - then ignore it
    //
    char*   attrName   = orionldContextItemExpand(orionldState.contextP, attrP->name, true, NULL);
    KjNode* dbEntityP  = entityInstanceLookup(dbEntityV, entityId, NULL);
    KjNode* attrNamesP = kjLookup(dbEntityP, "attrNames");
    KjNode* oldAttrP   = kjStringValueLookupInArray(attrNamesP, attrName);

    if (oldAttrP == NULL)
      oldAttrP = kjLookup(entityP, attrP->name);

    kjChildRemove(copyP, attrP);
    if (oldAttrP == NULL)  // Ignore the attribute, if it already existed
      kjChildAdd(entityP, attrP);

    attrP = next;
  }
}



// -----------------------------------------------------------------------------
//
// duplicatedInstances - remove (save in array for TRoE) duplicated entities from the array
//
// If more than ONE instance of an entity:
//   - For REPLACE - remove all entity instances but the last
//   - For UPDATE  - remove all instances and add a new one - merged from all of them
//
//
// REPLACE
//   If entities are replaced, then only the last one is relevant for the current state
//   All instances but the last are removed from the incoming tree
//   All removed instances are added to the array for TRoE
//
// UPDATE
//   If entities are updated, then none of the individual instances are relevant for the "current state"
//   All instances are removed and instead added to the array for TRoE - orionldState.duplicateArray
//   After that, all TRoE instances are merged into a resulting instance that is then added to the tree for "current state"
//
// So, for both cases we'll remove all instances that has a duplicate
// Then, for REPLACE, we'll put back the last, and
// for UPDATE, we merge them all into a new entity that is added to the original array
//
void duplicatedInstances(KjNode* incomingTree, KjNode* dbEntityV, bool entityReplace, bool attributeReplace, KjNode* errorsArray)
{
  KjNode* entityP  = incomingTree->value.firstChildP;
  KjNode* next     = NULL;

  //
  // For all entities that have more than one instance in 'incomingTree':
  //   - Move all instances to orionldState.duplicateArray (except if entityReplace == true)
  //     - IF (entityReplace == true): Keep the last instance in 'incomingTree' as entities replace eachother
  //   - If UPDATE - merge all instances into one single instance
  //
  while (entityP)
  {
    next = entityP->next;
    KjNode* idP = kjLookup(entityP, "id");

    if (idP == NULL)
    {
      LM_E(("Internal Error (no id field found for entity)"));
      entityP = next;
      continue;
    }

    char*   entityId = idP->value.s;
    KjNode* sameIdP  = entityInstanceLookup(incomingTree, entityId, entityP);

    if (entityReplace == false)  // For Updates (All 4 combinations except Upsert with ReplaceEntities) - ALL instances must go
    {
      if (orionldState.duplicateArray != NULL)
        sameIdP = entityIdLookup(orionldState.duplicateArray, entityId);
    }

    if (sameIdP == NULL)
    {
      entityP = next;
      continue;
    }

    if (orionldState.duplicateArray == NULL)
      orionldState.duplicateArray = kjArray(orionldState.kjsonP, NULL);

    entityErrorPush(errorsArray, entityId, OrionldBadRequestData, "Duplicated Entity", "previous instances merged into one", 400, true);
    kjChildRemove(incomingTree, entityP);
    kjChildAdd(orionldState.duplicateArray, entityP);
    entityP = next;
  }

  if (entityReplace == true)  // For ENTITY-REPLACE, we're done
    return;

  if (orionldState.duplicateArray == NULL)  // If no duplicates, we're done
    return;

  //
  // For UPDATE, we need to merge all instances in order into one new entity instance and put the merged result back into incomingTree
  //
  // For this algorithm, the entities in orionldState.duplicateArray are removed after beign merged.
  // However, this is no good for TRoE as the duplicated-array is needed for the TRoE database.
  // So, if TRoE is on, then we need to clone the array before starting with the merge
  //
  KjNode* duplicateArray = (troe == true)? kjClone(orionldState.kjsonP, orionldState.duplicateArray) : orionldState.duplicateArray;

  entityP = duplicateArray->value.firstChildP;
  while (entityP)
  {
    // Decouple the first entity and use it as base for the merge
    next = entityP->next;
    kjChildRemove(duplicateArray, entityP);

    KjNode* idP      = kjLookup(entityP, "id");
    char*   entityId = idP->value.s;

    // Find all other instances of the same entity and merge them all into entityP
    KjNode* copyP = next;
    KjNode* copyNext;
    KjNode* copyIdP;

    while (copyP != NULL)
    {
      copyNext = copyP->next;
      copyIdP  = kjLookup(copyP, "id");

      if (strcmp(copyIdP->value.s, entityId) == 0)
      {
        if (copyP == next)
          next = next->next;
        kjChildRemove(duplicateArray, copyP);
        if (attributeReplace == true)
          kjEntityMergeReplacingAttributes(entityP, copyP);
        else
          kjEntityMergeIgnoringExistingAttributes(entityP, entityId, dbEntityV, copyP);
      }

      copyP = copyNext;
    }

    // This merged entity should be ignored for TRoE as it is the merged result of all instances
    // and all instances are already present in orionldState.duplicateArray
    //
    kjChildAdd(incomingTree, entityP);
    troeIgnoreMark(entityP);
    entityP = next;
  }
}
