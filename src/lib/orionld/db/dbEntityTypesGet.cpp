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
#include "kjson/kjRender.h"                                       // kjRender
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
    if (prev->next == NULL)  // Insert as last item
    {
      prev->next       = newItemP;
      newItemP->next   = NULL;
      array->lastChild = newItemP;
    }
    else  // Insert the middle
    {
      newItemP->next = prev->next;
      prev->next     = newItemP;
    }
  }
  else  // Insert as first item
  {
    newItemP->next = array->value.firstChildP;
    array->value.firstChildP = newItemP;
  }
}



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
// typesAndAttributesExtractFromEntities -
//
static KjNode* typesAndAttributesExtractFromEntities(KjNode* array)
{
  KjNode* typeArray = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* entityP = array->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* nodeResponseP = kjObject(orionldState.kjsonP, NULL);
    KjNode* _idP          = kjLookup(entityP, "_id");
    KjNode* typeP         = kjLookup(_idP, "type");
    KjNode* attributesP   = kjLookup(entityP, "attrNames");

    if (typeP != NULL)
    {
      char*   typeName       = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
      KjNode* typeIdNodeP    = kjString(orionldState.kjsonP, "id", typeP->value.s);
      KjNode* typeNameNodeP  = kjString(orionldState.kjsonP, "typeName", typeName);

      kjChildAdd(nodeResponseP, typeIdNodeP);
      kjChildAdd(nodeResponseP, typeNameNodeP);
    }

    if (attributesP != NULL)
    {
      KjNode* attributeNamesNodeListP = kjArray(orionldState.kjsonP,  "attributeNames");
      KjNode* nodeP                   = attributesP->value.firstChildP;
      KjNode* next;

      while (nodeP != NULL)
      {
        next = nodeP->next;

        kjChildRemove(attributesP, nodeP);
        nodeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, nodeP->value.s, NULL, NULL);
        kjStringArraySortedInsert(attributeNamesNodeListP, nodeP);

        nodeP = next;
      }

      kjChildAdd(nodeResponseP, attributeNamesNodeListP);
    }

    kjChildAdd(typeArray, nodeResponseP);  // OK to break tree
  }

  return typeArray;
}



// -----------------------------------------------------------------------------
//
// typesAndAttributesExtractFromRegistrations -
//
static KjNode* typesAndAttributesExtractFromRegistrations(KjNode* array)
{
  KjNode* typeArray = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* registrationP = array->value.firstChildP; registrationP != NULL; registrationP = registrationP->next)
  {
    KjNode* responseNodeP = kjObject(orionldState.kjsonP, NULL);
    KjNode* typeP         = kjLookup(registrationP, "type");
    KjNode* attributesP   = kjLookup(registrationP, "attrs");

    if (typeP == NULL)
    {
      LM_E(("Internal Error (no 'type' item in registration)"));
      return NULL;
    }

    KjNode* idNodeP        = kjString(orionldState.kjsonP, "id", typeP->value.s);
    char*   typeName       = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
    KjNode* typeNameNodeP  = kjString(orionldState.kjsonP, "typeName", typeName);

    kjChildAdd(responseNodeP, idNodeP);
    kjChildAdd(responseNodeP, typeNameNodeP);

    if (attributesP != NULL)
    {
      KjNode* attributeNamesNodeListP = kjArray(orionldState.kjsonP,  "attributeNames");
      KjNode* nodeP                   = attributesP->value.firstChildP;
      KjNode* next;

      while (nodeP != NULL)
      {
        next = nodeP->next;

        kjChildRemove(attributesP, nodeP);
        nodeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, nodeP->value.s, NULL, NULL);
        kjStringArraySortedInsert(attributeNamesNodeListP, nodeP);

        nodeP = next;
      }

      kjChildAdd(responseNodeP, attributeNamesNodeListP);
    }

    kjChildAdd(typeArray, responseNodeP);
  }

  return typeArray;
}



// -----------------------------------------------------------------------------
//
// getEntityTypesResponse - All entity types for which entity instances are
// currently available in the NGSI-LD system.
//
static KjNode* getEntityTypesResponse(KjNode* sortedArrayP)
{
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



// -----------------------------------------------------------------------------
//
// getAvailableEntityTypesDetails - Details of all entity types for which entity
// instances are currently available in the NGSI-LD system.
//
static KjNode* getAvailableEntityTypesDetails(KjNode* sortedArrayP)
{
  KjNode* typeNodeDetailsListP = kjArray(orionldState.kjsonP,  NULL);

  for (KjNode* typeValueNodeP = sortedArrayP->value.firstChildP; typeValueNodeP != NULL; typeValueNodeP = typeValueNodeP->next)
  {
    KjNode* idP                = kjLookup(typeValueNodeP, "id");
    KjNode* typeNameP          = kjLookup(typeValueNodeP, "typeName");
    KjNode* attrsNameP         = kjLookup(typeValueNodeP, "attributeNames");
    KjNode* typeNodeResponseP  = kjObject(orionldState.kjsonP, NULL);

    if ((idP != NULL) && (typeNameP != NULL))
    {
      KjNode* idNodeP        = kjString(orionldState.kjsonP, "id", idP->value.s);
      KjNode* typeNodeP      = kjString(orionldState.kjsonP, "type", "EntityType");
      KjNode* typeNameNodeP  = kjString(orionldState.kjsonP, "typeName", typeNameP->value.s);

      kjChildAdd(typeNodeResponseP, idNodeP);
      kjChildAdd(typeNodeResponseP, typeNodeP);
      kjChildAdd(typeNodeResponseP, typeNameNodeP);
    }

    if (attrsNameP != NULL)
      kjChildAdd(typeNodeResponseP, attrsNameP);

    kjChildAdd(typeNodeDetailsListP, typeNodeResponseP);
  }
  return typeNodeDetailsListP;
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
  char*    fields[2];
  KjNode*  arrayP = NULL;

  fields[0] = (char*) "_id";

  if (orionldState.uriParams.details == false)
    local  = dbEntitiesGet(fields, 1);
  else
  {
    fields[1] = (char*) "attrNames";
    local  = dbEntitiesGet(fields, 2);
  }

  remote = dbEntityTypesFromRegistrationsGet();
  if ((remote != NULL) && (orionldState.uriParams.details == true))
    remote = typesAndAttributesExtractFromRegistrations(remote);

  if (local != NULL)
  {
    if (orionldState.uriParams.details == false)
      local = typesExtract(local);
    else
      local = typesAndAttributesExtractFromEntities(local);
  }

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

    //
    // Concatenate remote + local
    //
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

  if (orionldState.uriParams.details == false)
    return getEntityTypesResponse(sortedArrayP);

  return getAvailableEntityTypesDetails(sortedArrayP);
}
