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
#include <vector>                             // std::vector

extern "C"
{
#include "kbase/kMacros.h"                    // K_FT
#include "kjson/KjNode.h"                     // KjNode
#include "kjson/kjBuilder.h"                  // kjString, kjObject, ...
#include "kjson/kjLookup.h"                   // kjLookup
#include "kjson/kjRender.h"                   // kjRender
}

#include "logMsg/logMsg.h"                    // LM_*
#include "logMsg/traceLevels.h"               // Lmt*

#include "common/globals.h"                   // parse8601Time
#include "rest/ConnectionInfo.h"              // ConnectionInfo
#include "rest/httpHeaderAdd.h"               // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"        // orion::ValueType
#include "orionTypes/UpdateActionType.h"      // ActionType
#include "parse/CompoundValueNode.h"          // CompoundValueNode
#include "ngsi/ContextAttribute.h"            // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"      // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"     // UpdateContextResponse
#include "mongoBackend/mongoEntityExists.h"   // mongoEntityExists
#include "mongoBackend/mongoUpdateContext.h"  // mongoUpdateContext
#include "rest/uriParamNames.h"               // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
#include "mongoBackend/MongoGlobal.h"         // getMongoConnection()

#include "orionld/rest/orionldServiceInit.h"                   // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECK
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldAttributeTreat.h"              // orionldAttributeTreat
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"          // orionldUriExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"    // Own Interface
#include "orionld/db/dbEntityUpdateAttribute.h"                // dbEntityUpdateAttribute



// ----------------------------------------------------------------------------
//
// entityErrorPush -
//
// The array "errors" in BatchOperationResult is an array of BatchEntityError.
// BatchEntityError contains a string (the entity id) and an instance of ProblemDetails.
//
// ProblemDetails is described in https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf
// and contains:
//
// * type      (string) A URI reference that identifies the problem type
// * title     (string) A short, human-readable summary of the problem
// * detail    (string) A human-readable explanation specific to this occurrence of the problem
// * status    (number) The HTTP status code
// * instance  (string) A URI reference that identifies the specific occurrence of the problem
//
// Of these five items, only "type" seems to be mandatory.
//
// This implementation will treat "type", "title", and "status" as MANDATORY, and "detail" as OPTIONAL
//
static void entityErrorPush(KjNode *errorsArrayP, const char *entityId, OrionldResponseErrorType type, const char *title, const char *detail, int status)
{
  KjNode *objP            = kjObject(orionldState.kjsonP, NULL);
  KjNode *eIdP            = kjString(orionldState.kjsonP,  "entityId", entityId);
  KjNode *problemDetailsP = kjObject(orionldState.kjsonP,  "error");
  KjNode *typeP           = kjString(orionldState.kjsonP,  "type",     orionldErrorTypeToString(type));
  KjNode *titleP          = kjString(orionldState.kjsonP,  "title",    title);
  KjNode *statusP         = kjInteger(orionldState.kjsonP, "status",   status);

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



// ----------------------------------------------------------------------------
//
// entityIdPush - add ID to array
//
static void entityIdPush(KjNode *entityIdsArrayP, const char *entityId)
{
  KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, entityId);

  kjChildAdd(entityIdsArrayP, idNodeP);
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
// entityLookupById - lookup an entity in an array of entities, by its entity-id
//
static KjNode* entityLookupById(KjNode* entityArray, char* entityId)
{
  for (KjNode* entityP = entityArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* idNodeP = kjLookup(entityP, "id");

    if ((idNodeP != NULL) && (strcmp(idNodeP->value.s, entityId) == 0))  // If NULL, something is really wrong!!!
      return entityP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// entityTypeGet - lookup 'type' in a KjTree
//
static char* entityTypeGet(KjNode* entityNodeP)
{
  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
      return itemP->value.s;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// entityIdCheck -
//
static bool entityIdCheck(KjNode* entityIdNodeP, bool duplicatedId, KjNode* errorsArrayP)
{
  // Entity ID is mandatory
  if (entityIdNodeP == NULL)
  {
    LM_W(("Bad Input (UPSERT: mandatory field missing: entity::id)"));
    entityErrorPush(errorsArrayP, "no entity::id", OrionldBadRequestData, "mandatory field missing", "entity::id", 400);
    return false;
  }

  // Entity ID must be a string
  if (entityIdNodeP->type != KjString)
  {
    LM_W(("Bad Input (UPSERT: entity::id not a string)"));
    entityErrorPush(errorsArrayP, "invalid entity::id", OrionldBadRequestData, "field with invalid type", "entity::id", 400);
    return false;
  }

  // Entity ID must be a valid URI
  char* detail;
  if (!urlCheck(entityIdNodeP->value.s, &detail) && !urnCheck(entityIdNodeP->value.s, &detail))
  {
    LM_W(("Bad Input (UPSERT: entity::id is a string but not a valid URI)"));
    entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Not a URI", entityIdNodeP->value.s, 400);
    return false;
  }

  // Entity ID must not be duplicated
  if (duplicatedId == true)
  {
    LM_W(("Bad Input (UPSERT: Duplicated entity::id)"));
    entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Duplicated field", "entity::id", 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// entityTypeCheck -
//
static bool entityTypeCheck(KjNode* entityTypeNodeP, bool duplicatedType, char* entityId, bool typeMandatory, KjNode* errorsArrayP)
{
  // Entity TYPE is mandatory?
  if ((typeMandatory == true) && (entityTypeNodeP == NULL))
  {
    LM_W(("Bad Input (UPSERT: mandatory field missing: entity::type)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "mandatory field missing", "entity::type", 400);
    return false;
  }

  // Entity TYPE must not be duplicated
  if (duplicatedType == true)
  {
    LM_W(("KZ: Bad Input (UPSERT: Duplicated entity::type)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Duplicated field", "entity::type", 400);
    return false;
  }

  // Entity TYPE must be a string
  if (entityTypeNodeP->type != KjString)
  {
    LM_W(("Bad Input (UPSERT: entity::type not a string)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "field with invalid type", "entity::type", 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// entityIdAndTypeGet - lookup 'id' and 'type' in a KjTree
//
static bool entityIdAndTypeGet(KjNode* entityNodeP, char** idP, char** typeP, KjNode* errorsArrayP)
{
  KjNode*  idNodeP        = NULL;
  KjNode*  typeNodeP      = NULL;
  bool     idDuplicated   = false;
  bool     typeDuplicated = false;

  *idP   = NULL;
  *typeP = NULL;

  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE3(itemP->name, 'i', 'd', 0))
    {
      if (idNodeP != NULL)
        idDuplicated = true;
      else
        idNodeP = itemP;
    }
    else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
    {
      if (typeNodeP != NULL)
        typeDuplicated = true;
      else
        typeNodeP = itemP;
    }
  }

  if (entityIdCheck(idNodeP, idDuplicated, errorsArrayP) == false)
  {
    LM_E(("UPSERT: entityIdCheck flagged error"));
    return false;
  }

  *idP = idNodeP->value.s;

  if (typeNodeP != NULL)
  {
    if (entityTypeCheck(typeNodeP, typeDuplicated, idNodeP->value.s, false, errorsArrayP) == false)
    {
      LM_E(("UPSERT: entityTypeCheck flagged error"));
      return false;
    }

    *typeP = typeNodeP->value.s;
  }

  LM_E(("UPSERT: OK"));

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
  for (KjNode *itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
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

    // No key-values in batch ops - all attrs must be objects (except special fields 'creDate' and 'modDate'
    if (itemP->type != KjObject)
    {
      LM_E(("UPSERT: item '%s' is not a KjObject, but a '%s'", itemP->name, kjValueType(itemP->type)));
      *titleP  = (char*) "invalid entity";
      *detailP = (char*) "attribute must be a JSON object";

      return false;
    }

    KjNode*           attrTypeNodeP = NULL;
    ContextAttribute* caP           = new ContextAttribute();

    // orionldAttributeTreat treats the attribute, including expanding the attribute name and values, if applicable
    if (orionldAttributeTreat(ciP, itemP, caP, &attrTypeNodeP, detailP) == false)
    {
      LM_E(("orionldAttributeTreat failed"));
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
static void kjTreeToUpdateContextRequest(ConnectionInfo* ciP, UpdateContextRequest* ucrP, KjNode* treeP, KjNode* errorsArrayP)
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
    char*  entityId   = NULL;
    char*  entityType = NULL;
    char*  entityTypeExpanded;

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



// ----------------------------------------------------------------------------
//
// entitySuccessPush -
//
static void entitySuccessPush(KjNode *successArrayP, const char *entityId)
{
  KjNode *eIdP = kjString(orionldState.kjsonP, "id", entityId);

  kjChildAdd(successArrayP, eIdP);
}



// -----------------------------------------------------------------------------
//
// entityTypeAndCreDateGet -
//
static void entityTypeAndCreDateGet(KjNode* dbEntityP, char** idP, char** typeP, int* creDateP)
{
  for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE3(nodeP->name, 'i', 'd', 0))
      *idP = nodeP->value.s;
    else if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
      *typeP = nodeP->value.s;
    else if (SCOMPARE8(nodeP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
      *creDateP = nodeP->value.i;
  }
}



// -----------------------------------------------------------------------------
//
// removeArrayRemoveEntity -
//
KjNode* removeArrayEntityLookup(KjNode* removeArray, char* entityId)
{
  for (KjNode* nodeP = removeArray->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->value.s, entityId) == 0)
      return nodeP;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// typeCheckForNonExistingEntities -
//
bool typeCheckForNonExistingEntities(KjNode* incomingTree, KjNode* idTypeAndCreDateFromDb, KjNode* errorsArrayP, KjNode* removeArray)
{
  KjNode* inNodeP = incomingTree->value.firstChildP;
  KjNode* next;

  while (inNodeP != NULL)
  {
    //
    // entities that weren't found in the DB MUST contain entity::type
    //
    KjNode* inEntityIdNodeP = kjLookup(inNodeP, "id");

    if (inEntityIdNodeP == NULL)  // Entity ID is mandatory
    {
      LM_E(("KZ: Invalid Entity: Mandatory field entity::id is missing"));
      entityErrorPush(errorsArrayP, "No ID", OrionldBadRequestData, "Invalid Entity", "Mandatory field entity::id is missing", 400);
      next = inNodeP->next;
      kjChildRemove(incomingTree, inNodeP);
      inNodeP = next;
      continue;
    }

    KjNode* dbEntityId = NULL;

    // Lookup the entity::id in what came from the database - if anything at all came
    if (idTypeAndCreDateFromDb != NULL)
      dbEntityId = entityLookupById(idTypeAndCreDateFromDb, inEntityIdNodeP->value.s);

    if (dbEntityId == NULL)  // This Entity is to be created - "type" is mandatory!
    {
      KjNode* inEntityTypeNodeP = kjLookup(inNodeP, "type");

      if (inEntityTypeNodeP == NULL)
      {
        LM_E(("KZ: Invalid Entity: Mandatory field entity::type is missing"));
        entityErrorPush(errorsArrayP, inEntityIdNodeP->value.s, OrionldBadRequestData, "Invalid Entity", "Mandatory field entity::type is missing", 400);

        KjNode* entityInRemoveArray = removeArrayEntityLookup(removeArray, inEntityIdNodeP->value.s);
        if (entityInRemoveArray != NULL)
          kjChildRemove(removeArray, entityInRemoveArray);

        next = inNodeP->next;
        kjChildRemove(incomingTree, inNodeP);
        inNodeP = next;
        continue;
      }
    }

    inNodeP = inNodeP->next;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostEntityOperationsUpsert -
//
// POST /ngsi-ld/v1/entityOperations/upsert
//
// From the spec:
//   This operation allows creating a batch of NGSI-LD Entities, updating each of them if they already exist.
//
//   An optional flag indicating the update mode (only applies in case the Entity already exists):
//     - ?options=replace  (default)
//     - ?options=update
//
//   Replace:  All the existing Entity content shall be replaced  - like PUT
//   Update:   Existing Entity content shall be updated           - like PATCH
//
bool orionldPostBatchUpsert(ConnectionInfo* ciP)
{
  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array
  // * cannot be empty
  // * all entities must contain a entity::id (one level down)
  // * no entity can contain an entity::type (one level down)
  //
  ARRAY_CHECK(orionldState.requestTree, "toplevel");
  EMPTY_ARRAY_CHECK(orionldState.requestTree, "toplevel");

  //
  // Prerequisites for URI params:
  // * both 'update' and 'replace' cannot be set in options (replace is default)
  //
  if ((orionldState.uriParamOptions.update == true) && (orionldState.uriParamOptions.replace == true))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "URI Param Error", "options: both /update/ and /replace/ present");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }


  //
  // if (replace)
  // {
  //   01. Create "idArray" from the "incomingTree", with error handling
  //   02. Get "idTypeAndCredateFromDb" by calling dbQueryEntitiesAsKjTree(idArray);
  //   03. Creation Date from DB entities, and type-check
  //       - Make sure that no entity in the incomingTree contains a "type" != type in db for that entity
  //       - Add creDate from DB to incomingTree
  //       Foreach entity in idTypeAndCredateFromDb  (those that existed in the database)
  //       03.1 Lookup entity-pointer in "incomingTree"
  //       03.2 Call entityFieldsExtract
  //       03.3 If entityFieldsExtract returns false:
  //            03.3.1 remove entity from incomingTree
  //            03.3.1 remove entity from idArray
  //            03.3.2 add error by calling entityErrorPush()
  //       03.4 Get type and creDate from entity in idTypeAndCredateFromDb
  //       03.5 Compare types, if entity in idTypeAndCredateFromDb has one and if not the same:
  //            03.5.1 remove entity from incomingTree
  //            03.5.1 remove entity from idArray
  //            03.5.2 add error by calling entityErrorPush()
  //       03.6 Add creDate to entity-pointer in "incomingTree"
  //       04. Make sure all entities that did not exist hav a type in the incoming payload
  //       05. Remove the entities in "idArray" from DB
  // }
  //
  // 06. Fill in UpdateContextRequest from "incomingTree"
  // 07. Set 'modDate' as "RIGHT NOW" for all entities
  // 08. Call mongoBackend with AppendStrict - as all already existing entities have been removed
  //
  KjNode*               incomingTree   = orionldState.requestTree;
  KjNode*               idArray        = kjArray(orionldState.kjsonP, NULL);
  KjNode*               successArrayP  = kjArray(orionldState.kjsonP, "success");
  KjNode*               errorsArrayP   = kjArray(orionldState.kjsonP, "errors");
  KjNode*               entityP;
  KjNode*               next;

  //
  // 01. Create idArray as an array of entity IDs, extracted from orionldState.requestTree
  //
  entityP = incomingTree->value.firstChildP;
  while (entityP)
  {
    next = entityP->next;

    char* entityId;
    char* entityType;

    // entityIdAndTypeGet calls entityIdCheck/entityTypeCheck that adds the entity in errorsArrayP if needed
    if (entityIdAndTypeGet(entityP, &entityId, &entityType, errorsArrayP) == true)
      entityIdPush(idArray, entityId);
    else
      kjChildRemove(incomingTree, entityP);

    entityP = next;
  }


  //
  // 02. Query database extracting three fields: { id, type and creDate } for each of the entities
  //     whose Entity::Id is part of the array "idArray".
  //     The result is "idTypeAndCredateFromDb" - an array of "tiny" entities with { id, type and creDate }
  //
  KjNode* idTypeAndCreDateFromDb = dbQueryEntitiesAsKjTree(idArray);

  //
  // 03. Creation Date from DB entities, and type-check
  //
  // LOOP OVER idTypeAndCreDateFromDb.
  // Add all the entities to "removeArray", unless an error occurs (with non-matching types for example)
  //
  KjNode* removeArray       = NULL;  // This array contains the Entity::Id of all entities to be removed from DB
  int     entitiesToRemove  = 0;

  if (idTypeAndCreDateFromDb != NULL)
  {
    for (KjNode* dbEntityP = idTypeAndCreDateFromDb->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      char*    idInDb        = NULL;
      char*    typeInDb      = NULL;
      int      creDateInDb   = 0;
      char*    typeInPayload = NULL;
      KjNode*  entityP;

      // Get entity id, type and creDate from the DB
      entityTypeAndCreDateGet(dbEntityP, &idInDb, &typeInDb, &creDateInDb);

      //
      // For the entity in question - get id and type from the incoming payload
      // First look up the entity with ID 'idInDb' in the incoming payload
      //
      entityP       = entityLookupById(incomingTree, idInDb);
      typeInPayload = entityTypeGet(entityP);


      //
      // If type exists in the incoming payload, it must be equal to the type in the DB
      // If not, it's an error, so:
      //   - add entityId to errorsArrayP
      //   - add entityId to removeArray
      //   - remove from incomingTree
      //
      // Remember, the type in DB is expanded. We must expand the 'type' in the incoming payload as well, before we compare
      //
      if (typeInPayload != NULL)
      {
        char* typeInPayloadExpanded = orionldContextItemExpand(orionldState.contextP, typeInPayload, NULL, true, NULL);

        if (strcmp(typeInPayloadExpanded, typeInDb) != 0)
        {
          //
          // As the entity type differed, this entity will not be updated in DB, nor will it be removed:
          // - removed from incomingTree
          // - not added to "removeArray"
          //
          entityErrorPush(errorsArrayP, idInDb, OrionldBadRequestData, "non-matching entity type", typeInPayload, 400);
          kjChildRemove(incomingTree, entityP);
          entityP = next;
          continue;
        }
      }
      else
      {
        // Add 'type' to entity in incoming tree, if necessary
        KjNode* typeNodeP = kjString(orionldState.kjsonP, "type", typeInDb);
        kjChildAdd(entityP, typeNodeP);
      }

      //
      // Add creDate from DB to the entity of the incoming tree
      //
      KjNode* creDateNodeP = kjInteger(orionldState.kjsonP, idInDb, creDateInDb);
      if (orionldState.creDatesP == NULL)
        orionldState.creDatesP = kjObject(orionldState.kjsonP, NULL);
      kjChildAdd(orionldState.creDatesP, creDateNodeP);

      //
      // Add the Entity-ID to "removeArray" for later removal, before re-creation
      //
      if (removeArray == NULL)
        removeArray = kjArray(orionldState.kjsonP, NULL);

      KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, idInDb);
      kjChildAdd(removeArray, idNodeP);
      ++entitiesToRemove;

      // Point to the next entity and continue
      entityP = next;
    }
  }


  //
  // 04. Entity::type is MANDATORY for entities that did not already exist
  //     Erroneous entities must be:
  //     - reported via entityErrorPush()
  //     - removed from "removeArray"
  //     - removed from "incomingTree"
  //
  // So, before calling 'typeCheckForNonExistingEntities' we must make sure the removeArray exists
  //
  if (removeArray == NULL)
    removeArray = kjArray(orionldState.kjsonP, NULL);

  typeCheckForNonExistingEntities(incomingTree, idTypeAndCreDateFromDb, errorsArrayP, removeArray);


  //
  // 05. Remove the entities in "removeArray" from DB
  //
  if ((removeArray != NULL) && (removeArray->value.firstChildP != NULL))
    dbEntityBatchDelete(removeArray);


  //
  // 06. Fill in UpdateContextRequest from "incomingTree"
  //
  UpdateContextRequest  mongoRequest;

  mongoRequest.updateActionType = ActionTypeAppendStrict;

  kjTreeToUpdateContextRequest(ciP, &mongoRequest, incomingTree, errorsArrayP);


  //
  // 07. Set 'modDate' as "RIGHT NOW"
  //
  time_t now = time(NULL);

  for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.size(); ++ix)
  {
    mongoRequest.contextElementVector[ix]->entityId.modDate = now;
  }


  //
  // 08. Call mongoBackend - to create/modify the entities
  //     In case of REPLACE, all entities have been removed from the DB prior to this call, so, they will all be created.
  //
  UpdateContextResponse mongoResponse;

  ciP->httpStatusCode = mongoUpdateContext(&mongoRequest,
                                           &mongoResponse,
                                           orionldState.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);

  //
  // Now check orionldState.errorAttributeArray to see whether any attribute failed to be updated
  //
  // bool partialUpdate = (orionldState.errorAttributeArrayP[0] == 0)? false : true;
  // bool retValue      = true;
  //

  if (ciP->httpStatusCode == SccOk)
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
    {
      const char *entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

      if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == SccOk)
        entitySuccessPush(successArrayP, entityId);
      else
        entityErrorPush(errorsArrayP,
                        entityId,
                        OrionldBadRequestData,
                        "",
                        mongoResponse.contextElementResponseVector.vec[ix]->statusCode.reasonPhrase.c_str(),
                        400);
    }

    for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.vec.size(); ix++)
    {
      const char *entityId = mongoRequest.contextElementVector.vec[ix]->entityId.id.c_str();

      if (kjStringValueLookupInArray(successArrayP, entityId) == NULL)
        entitySuccessPush(successArrayP, entityId);
    }

    //
    // Add the success/error arrays to the response-tree
    //
    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);

    ciP->httpStatusCode = SccOk;
  }

  mongoRequest.release();
  mongoResponse.release();

  if (ciP->httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext flagged an error"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Database Error");
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  return true;
}
