/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <unistd.h>                                               // NULL

extern "C"
{
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjBuilder.h"                                      // kjArray
#include "kjson/kjLookup.h"                                       // kjLookup
#include "kjson/kjRender.h"                                       // kjRender - TMP
}

#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                  // OrionldProblemDetails
#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"        // orionldContextItemAliasLookup
#include "orionld/db/dbConfiguration.h"                           // dbEntityTypesFromRegistrationsGet, dbEntitiesGet
#include "orionld/db/dbEntityTypesGet.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// typesExtract -
//
static KjNode* typesExtract(KjNode* array)
{
  KjNode* typeArray = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* entityP = array->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* idP   = entityP->value.firstChildP;  // The entities has a single child '_id'
    KjNode* typeP = kjLookup(idP, "type");

    if (typeP != NULL)
    {
      kjChildAdd(typeArray, typeP);  // OK to break tree, as idP is one level up and its next pointer is still intact
      typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
    }
  }

  return typeArray;
}



// -----------------------------------------------------------------------------
//
// kjStringArraySortedInsert -
//
static void kjStringArraySortedInsert(KjNode* array, KjNode* newItemP)
{
  KjNode* prev  = NULL;
  KjNode* itemP = array->value.firstChildP;

  while (itemP != NULL)
  {
    int cmp = strcmp(itemP->value.s, newItemP->value.s);  // <0 if itemP < newItemP,  ==0 id equal

    if (cmp < 0)
      prev = itemP;
    else if (cmp == 0)
    {
      LM_TMP(("ET: Trowing away a '%s' as it's already present", newItemP->value.s));
      return;  // We skip values that are already present
    }

    itemP = itemP->next;
  }

  if (prev != NULL)
  {
    if (prev->next == NULL)  // insert as last item
    {
      prev->next = newItemP;
      newItemP->next = NULL;
      array->lastChild = newItemP;
    }
    else  // In the middle
    {
      newItemP->next = prev->next;
      prev->next = newItemP;
    }
  }
  else  // insert as first item
  {
    newItemP->next = array->value.firstChildP;
    array->value.firstChildP = newItemP;
  }
}



// ----------------------------------------------------------------------------
//
// dbEntityTypesGet -
//
// FIXME: Move to db library - no direct DB handling is done here
//
KjNode* dbEntityTypesGet(OrionldProblemDetails* pdP)
{
  KjNode*  local;
  KjNode*  remote;
  char*    fields[1];
  KjNode*  arrayP = NULL;

  fields[0] = (char*) "_id";
  
  local  = dbEntitiesGet(fields, 1);
  remote = dbEntityTypesFromRegistrationsGet();

  if (local != NULL)
    local = typesExtract(local);

  LM_TMP(("ET: remote at %p", remote));
  LM_TMP(("ET: local  at %p", local));

  if ((remote == NULL) && (local == NULL))
  {
    KjNode* emptyArray = kjArray(orionldState.kjsonP, NULL);
    return emptyArray;
  }
  else if (remote == NULL)
    arrayP = local;
  else if (local == NULL)
    arrayP = remote;
  else
  {
    arrayP = remote;

    remote->lastChild->next  = local->value.firstChildP;
    remote->lastChild        = local->lastChild;
  }

  // Sort
  KjNode* sortedArrayP = kjArray(orionldState.kjsonP, NULL);

  char buf[1024];
  kjRender(orionldState.kjsonP, arrayP, buf, 1024);
  LM_TMP(("ET: In-Array: %s", buf));

  //
  // The very first item can be inserted directly, without caring about sorting
  // This is faster and it also makes the sorting algorithm a little easier as the out-array is never empty
  //
  KjNode* firstChild = arrayP->value.firstChildP;
  kjChildRemove(arrayP, firstChild);
  kjChildAdd(sortedArrayP, firstChild);

  //
  // Looping over arrayP, removing all items and inserting them in sortedArray.
  // Duplicated items are skipped.
  //
  KjNode* nodeP = arrayP->value.firstChildP;
  KjNode* next;
  while (nodeP != NULL)
  {
    next = nodeP->next;

    kjChildRemove(arrayP, nodeP);
    kjStringArraySortedInsert(sortedArrayP, nodeP);

    kjRender(orionldState.kjsonP, sortedArrayP, buf, 1024);
    LM_TMP(("ET: Sorted Array: %s", buf));

    nodeP = next;
  }
  LM_TMP(("ET: Done"));

  kjRender(orionldState.kjsonP, sortedArrayP, buf, 1024);
  LM_TMP(("ET: Out-Array: %s", buf));

  return sortedArrayP;
}
