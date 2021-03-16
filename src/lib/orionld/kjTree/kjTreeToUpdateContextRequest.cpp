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
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/entityErrorPush.h"                      // entityErrorPush
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"         // Own interface



// -----------------------------------------------------------------------------
//
// entityFieldsExtractSimple -
//
// This function is a simpler varianmt of 'entityFieldsExtract'.
// No need to check for duplicates, already done.
// We only need to lookup id and type and remove them
//
static bool entityFieldsExtractSimple(KjNode* entityNodeP, char** entityIdP, char** entityTypeP, KjNode** contextPP)
{
  KjNode*  itemP = entityNodeP->value.firstChildP;

  while (itemP != NULL)
  {
    KjNode* next = itemP->next;

    if (SCOMPARE3(itemP->name, 'i', 'd', 0) || SCOMPARE4(itemP->name, '@', 'i', 'd', 0))
    {
      *entityIdP = itemP->value.s;
      kjChildRemove(entityNodeP, itemP);
    }
    else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0) || SCOMPARE6(itemP->name, '@', 't', 'y', 'p', 'e', 0))
    {
      *entityTypeP = itemP->value.s;
      kjChildRemove(entityNodeP, itemP);
    }
    else if (SCOMPARE9(itemP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      *contextPP = itemP;
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
// NOTE: "id", "type", and "@context" of the entity must be removed from the tree before this function is called
//
static bool kjTreeToContextElementAttributes
(
  OrionldContext*  contextP,
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
    if (itemP == createdAtP)
      continue;
    else if (itemP == modifiedAtP)
      continue;
    else if (SCOMPARE8(itemP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
      continue;
    else if (SCOMPARE8(itemP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
      continue;
    else if (SCOMPARE10(itemP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
      continue;
    else if (SCOMPARE11(itemP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
      continue;
    else if (itemP->type != KjObject)  // No key-values in batch ops - all attrs must be objects (except special fields 'creDate' and 'modDate')
    {
      LM_E(("Attribute '%s' is not a KjObject, but a '%s'", itemP->name, kjValueType(itemP->type)));
      *titleP  = (char*) "invalid entity";
      *detailP = (char*) "attribute must be a JSON object";

      return false;
    }
    else
    {
      KjNode*           attrTypeNodeP = NULL;
      ContextAttribute* caP           = new ContextAttribute();

      // kjTreeToContextAttribute treats the attribute, including expanding the attribute name and values, if applicable
      if (kjTreeToContextAttribute(contextP, itemP, caP, &attrTypeNodeP, detailP) == false)
      {
        // kjTreeToContextAttribute calls orionldErrorResponseCreate
        LM_E(("kjTreeToContextAttribute failed for attribute '%s': %s", itemP->name, *detailP));
        *titleP = (char*) "Error treating attribute";
        delete caP;

        return false;
      }

      ceP->contextAttributeVector.push_back(caP);
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToUpdateContextRequest -
//
// NOTE
//   treeP is supposed to be an Array of Entities
//   This function is called only by BATCH Create/Update/Upsert
//
// If Content-Type is "application/ld+json", then the @context must be part of each and every item of the array
//
void kjTreeToUpdateContextRequest(UpdateContextRequest* ucrP, KjNode* treeP, KjNode* errorsArrayP, KjNode* idTypeAndCreDateFromDb)
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
      entityErrorPush(errorsArrayP, "No Entity ID", OrionldBadRequestData, "Entity must be a JSON Object", kjValueType(entityP->type), 400, false);
      entityP = next;
      continue;
    }

    char*           title;
    char*           detail;
    char*           entityId           = NULL;
    char*           entityType         = NULL;
    char*           entityTypeExpanded = NULL;
    KjNode*         contextNodeP       = NULL;
    OrionldContext* contextP           = orionldState.contextP;

    entityFieldsExtractSimple(entityP, &entityId, &entityType, &contextNodeP);

    if (orionldState.ngsildContent == true)
    {
      if (contextNodeP == NULL)
      {
        LM_E(("Content-Type is 'application/ld+json', but no @context found for entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Content-Type is 'application/ld+json', but no @context in payload data array item", NULL, 400, false);
        entityP = next;
        continue;
      }

      OrionldProblemDetails pd;
      contextP = orionldContextFromTree(NULL, false, contextNodeP, &pd);

      if (contextP == NULL)
      {
        LM_E(("orionldContextFromTree reports error: %s: %s", pd.title, pd.detail));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, pd.title, pd.detail, pd.status, false);
        entityP = next;
        continue;
      }
    }
    else
    {
      if (contextNodeP != NULL)
      {
        LM_E(("Content-Type is 'application/json', and an @context is present in the payload data array item of entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Content-Type is 'application/json', and an @context is present in the payload data array item", NULL, 400, false);
        entityP = next;
        continue;
      }
      else
        contextP = orionldState.contextP;
    }

    if (entityType == NULL)
    {
      //
      // No entity type given in incoming payload - we'll have to look it up in the database
      //
      // If a NULL is passed as idTypeAndCreDateFromDb, then error
      //
      if (idTypeAndCreDateFromDb == NULL)
      {
        LM_E(("No entity type given and no way to find it in the DB - we should never get here"));
        entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "No entity type given and no way to find it in the DB", "we should never get here", 500, false);
        entityP = next;
        continue;
      }

      KjNode* dbEntityP = NULL;
      for (KjNode* eP = idTypeAndCreDateFromDb->value.firstChildP; eP != NULL; eP = eP->next)
      {
        KjNode* idP = kjLookup(eP, "id");

        if (idP == NULL)
        {
          LM_E(("Internal Error (no 'id' field found in entity taken from DB)"));
          continue;
        }

        if (idP->type != KjString)
        {
          LM_E(("Internal Error ('id' field found in entity taken from DB but it's not a KjString (%s))", kjValueType(idP->type)));
          continue;
        }

        if (strcmp(idP->value.s, entityId) == 0)
        {
          dbEntityP = eP;
          break;
        }
      }

      if (dbEntityP == NULL)
      {
        LM_E(("Internal Error (no entity type in payload and entity not found in DB - this should have been reported as an error in an earlier stage ...)"));
        entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "no entity type in payload and entity not found in DB", NULL, 500, false);
        entityP = next;
        continue;
      }

      KjNode* dbEntityTypeP = kjLookup(dbEntityP, "type");
      if (dbEntityTypeP == NULL)
      {
        LM_E(("Internal Error (no entity type in object extracted from the DB)"));
        entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "no entity type in object extracted from the DB", NULL, 500, false);
        entityP = next;
        continue;
      }

      entityType = dbEntityTypeP->value.s;
    }

    entityTypeExpanded = orionldContextItemExpand(contextP, entityType, true, NULL);  // entity type
    if (entityTypeExpanded == NULL)
    {
      LM_E(("orionldContextItemExpand failed for '%s': %s", entityType, detail));
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "unable to expand entity::type", detail, 400, false);
      entityP = next;
      continue;
    }

    ContextElement* ceP          = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
    EntityId*       entityIdP    = &ceP->entityId;

    entityIdP->id        = entityId;
    entityIdP->type      = entityTypeExpanded;
    entityIdP->isPattern = "false";

    if (kjTreeToContextElementAttributes(contextP, entityP, NULL, NULL, ceP, &title, &detail) == false)
    {
      LM_W(("kjTreeToContextElementAttributes flags error '%s: %s' for entity '%s'", title, detail, entityId));
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, title, detail, 400, false);
      delete ceP;
      entityP = next;
      continue;
    }

    ucrP->contextElementVector.push_back(ceP);

    //
    // FIXME: This is where the id and type would be put back for TRoE
    //
    entityP = next;
  }
}
