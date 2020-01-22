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
* Author: Gabriel Quaresma and Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"         // Own interface



// -----------------------------------------------------------------------------
//
// entityErrorPush -
//
static void entityErrorPush(KjNode* errorsArrayP, const char* entityId, OrionldResponseErrorType type, const char* title, const char* detail, int status)
{
  KjNode* objP            = kjObject(orionldState.kjsonP, NULL);
  KjNode* eIdP            = kjString(orionldState.kjsonP,  "entityId", entityId);
  KjNode* problemDetailsP = kjObject(orionldState.kjsonP,  "error");
  KjNode* typeP           = kjString(orionldState.kjsonP,  "type",     orionldErrorTypeToString(type));
  KjNode* titleP          = kjString(orionldState.kjsonP,  "title",    title);
  KjNode* statusP         = kjInteger(orionldState.kjsonP, "status",   status);

  kjChildAdd(problemDetailsP, typeP);
  kjChildAdd(problemDetailsP, titleP);

  if (detail != NULL)
  {
    KjNode* detailP = kjString(orionldState.kjsonP, "detail", detail);
    kjChildAdd(problemDetailsP, detailP);
  }

  kjChildAdd(problemDetailsP, statusP);

  kjChildAdd(objP, eIdP);
  kjChildAdd(objP, problemDetailsP);

  kjChildAdd(errorsArrayP, objP);
}



// -----------------------------------------------------------------------------
//
// entityFieldsExtractSimple -
//
// This function is a simpler varianmt of 'entityFieldsExtract'.
// No need to check for duplicates, already done.
// We only need to lookup id and type and remove them
//
static bool entityFieldsExtractSimple(KjNode* entityNodeP, char** entityIdP, char** entityTypeP)
{
  KjNode*  itemP = entityNodeP->value.firstChildP;

  while (itemP != NULL)
  {
    KjNode* next = itemP->next;

    if (SCOMPARE3(itemP->name, 'i', 'd', 0))
    {
      *entityIdP = itemP->value.s;
      kjChildRemove(entityNodeP, itemP);
    }
    else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
    {
      *entityTypeP = itemP->value.s;
      kjChildRemove(entityNodeP, itemP);
    }

    itemP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToContextElementAttributes -
//
// NOTE: "id" and "type" of the entity must be removed from the tree before this function is called
//
static bool kjTreeToContextElementAttributes
(
  ConnectionInfo*  ciP,
  KjNode*          entityNodeP,
  KjNode*          createdAtP,
  KjNode*          modifiedAtP,
  ContextElement*  ceP,
  char**           titleP,
  char**           detailP
)
{
  // Iterate over the items of the entity
  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (createdAtP != NULL)
    {
      if (itemP == createdAtP)
        continue;
    }
    else if (SCOMPARE8(itemP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
      continue;

    if (modifiedAtP != NULL)
    {
      if (itemP == modifiedAtP)
        continue;
    }
    else if (SCOMPARE8(itemP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
      continue;

    // No key-values in batch ops - all attrs must be objects (except special fields 'creDate' and 'modDate')
    if (itemP->type != KjObject)
    {
      LM_E(("UPSERT: item '%s' is not a KjObject, but a '%s'", itemP->name, kjValueType(itemP->type)));
      *titleP  = (char*) "invalid entity";
      *detailP = (char*) "attribute must be a JSON object";

      return false;
    }

    KjNode*           attrTypeNodeP = NULL;
    ContextAttribute* caP           = new ContextAttribute();

    // kjTreeToContextAttribute treats the attribute, including expanding the attribute name and values, if applicable
    if (kjTreeToContextAttribute(ciP, itemP, caP, &attrTypeNodeP, detailP) == false)
    {
      // kjTreeToContextAttribute calls orionldErrorResponseCreate
      LM_E(("kjTreeToContextAttribute failed"));
      delete caP;
      return false;
    }

    ceP->contextAttributeVector.push_back(caP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToUpdateContextRequest -
//
// NOTE
//   treeP is supposed to be an Array of Entities
//
void kjTreeToUpdateContextRequest(ConnectionInfo* ciP, UpdateContextRequest* ucrP, KjNode* treeP, KjNode* errorsArrayP)
{
  KjNode* next;
  KjNode* entityP = treeP->value.firstChildP;

  while (entityP != NULL)
  {
    next = entityP->next;

    if (entityP->type != KjObject)
    {
      // Is this even possible???
      LM_E(("Entity not a JSON object!"));
      entityErrorPush(errorsArrayP, "No Entity ID", OrionldBadRequestData, "Entity must be a JSON Object", kjValueType(entityP->type), 400);
      entityP = next;
      continue;
    }

    char*  title;
    char*  detail;
    char*  entityId           = NULL;
    char*  entityType         = NULL;
    char*  entityTypeExpanded = NULL;

    entityFieldsExtractSimple(entityP, &entityId, &entityType);

    entityTypeExpanded = orionldContextItemExpand(orionldState.contextP, entityType, NULL, true, NULL);
    if (entityTypeExpanded == NULL)
    {
      LM_E(("orionldContextItemExpand failed for '%s': %s", entityType, detail));
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "unable to expand entity::type", detail, 400);
      entityP = next;
      continue;
    }

    ContextElement* ceP        = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
    EntityId*       entityIdP  = &ceP->entityId;

    entityIdP->id        = entityId;
    entityIdP->type      = entityTypeExpanded;
    entityIdP->isPattern = "false";

    if (kjTreeToContextElementAttributes(ciP, entityP, NULL, NULL, ceP, &title, &detail) == false)
    {
      LM_W(("kjTreeToContextElementAttributes flags error '%s: %s' for entity '%s'", title, detail, entityId));
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, title, detail, 400);
      delete ceP;
      entityP = next;
      continue;
    }

    ucrP->contextElementVector.push_back(ceP);
    entityP = next;
  }
}
