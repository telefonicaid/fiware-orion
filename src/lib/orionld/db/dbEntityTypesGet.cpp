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
#include "kjson/kjBuilder.h"                                      // kjArray, kjObject
#include "kjson/kjLookup.h"                                       // kjLookup
}

#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                  // OrionldProblemDetails
#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/uuidGenerate.h"                          // uuidGenerate
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
      return;  // We skip values that are already present

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
  KjNode* sortedArrayP = kjArray(orionldState.kjsonP, "typeList");

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

    nodeP = next;
  }

  char entityTypesId[64];
  strncpy(entityTypesId, "urn:ngsi-ld:EntityTypeList:", sizeof(entityTypesId));
  uuidGenerate(&entityTypesId[27]);

  KjNode* typeNodeResponseP = kjObject(orionldState.kjsonP, NULL);
  KjNode* idNodeP           = kjString(orionldState.kjsonP, "id", entityTypesId);
  KjNode* typeNodeP         = kjString(orionldState.kjsonP, "type", "EntityTypeList");

  kjChildAdd(typeNodeResponseP, idNodeP);
  kjChildAdd(typeNodeResponseP, typeNodeP);
  kjChildAdd(typeNodeResponseP, sortedArrayP);

  return typeNodeResponseP;
}
