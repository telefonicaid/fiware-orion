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
#include "kbase/kMacros.h"                                     // K_FT
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "common/globals.h"                                    // parse8601Time
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                         // orion::ValueType
#include "orionTypes/UpdateActionType.h"                       // ActionType
#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                       // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                      // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                   // mongoUpdateContext
#include "rest/uriParamNames.h"                                // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection()

#include "orionld/rest/orionldServiceInit.h"                   // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECK
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityIdCheck.h"                      // entityIdCheck
#include "orionld/common/entityTypeCheck.h"                    // entityTypeCheck
#include "orionld/common/entityIdAndTypeGet.h"                 // entityIdAndTypeGet
#include "orionld/common/entityLookupById.h"                   // entityLookupById
#include "orionld/common/typeCheckForNonExistingEntities.h"    // typeCheckForNonExistingEntities
#include "orionld/types/OrionldProblemDetails.h"               // OrionldProblemDetails
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldContextFromTree.h"            // orionldContextFromTree
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjEntityIdLookupInEntityArray.h"      // kjEntityIdLookupInEntityArray
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"       // kjTreeToUpdateContextRequest
#include "orionld/kjTree/kjEntityIdArrayExtract.h"             // kjEntityIdArrayExtract
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"    // Own Interface



// -----------------------------------------------------------------------------
//
// entityTypeGet - lookup 'type' in a KjTree
//
static char* entityTypeGet(KjNode* entityNodeP, KjNode** contextNodePP)
{
  char* type = NULL;

  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0) || SCOMPARE6(itemP->name, '@', 't', 'y', 'p', 'e', 0))
      type = itemP->value.s;
    if (SCOMPARE9(itemP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
      *contextNodePP = itemP;
  }

  return type;
}



// -----------------------------------------------------------------------------
//
// entityTypeAndCreDateGet -
//
static void entityTypeAndCreDateGet(KjNode* dbEntityP, char** idP, char** typeP, double* creDateP)
{
  for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE3(nodeP->name, 'i', 'd', 0) || SCOMPARE4(nodeP->name, '@', 'i', 'd', 0))
      *idP = nodeP->value.s;
    else if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0) || SCOMPARE6(nodeP->name, '@', 't', 'y', 'p', 'e', 0))
      *typeP = nodeP->value.s;
    else if (SCOMPARE8(nodeP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
    {
      if (nodeP->type == KjFloat)
        *creDateP = nodeP->value.f;
      else if (nodeP->type == KjInt)
        *creDateP = (double) nodeP->value.i;
    }
  }
}



// -----------------------------------------------------------------------------
//
// entityInstanceLookup -
//
KjNode* entityInstanceLookup(KjNode* array, const char* entityId, KjNode* entityP)
{
  LM_TMP(("IN"));
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
// entityIdLookup -
//
KjNode* entityIdLookup(KjNode* array, const char* entityId)
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
// duplicatedInstances - remove (save in array for temporal) duplicated entities from the array
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
void duplicatedInstances(KjNode* incomingTree, bool replace, KjNode* errorsArray)
{
  KjNode* entityP  = incomingTree->value.firstChildP;
  KjNode* next     = NULL;

  //
  // Remove all entities that have more than one instance
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

    if (replace == false)  // For Updates - ALL instances must go
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

  if (replace == true)  // For REPLACE, we're done
    return;

  if (orionldState.duplicateArray == NULL)  // If no duplicates, we're done
    return;

  // For UPDATE, we need to merge all instances in order into one new entity instance and put the merged result back into incomingTree
  entityP = orionldState.duplicateArray->value.firstChildP;
  while (entityP)
  {
    LM_TMP(("Here: entityP == %p", entityP));
    // Decouple the first entity and use it as base for the merge
    next = entityP->next;
    kjChildRemove(orionldState.duplicateArray, entityP);

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
        kjChildRemove(orionldState.duplicateArray, copyP);
        kjEntityMergeReplacingAttributes(entityP, copyP);
      }

      copyP = copyNext;
    }
    kjChildAdd(incomingTree, entityP);
    entityP = next;
  }
}



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpsert -
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
  // Prerequisites for URI params:
  // * both 'update' and 'replace' cannot be set in options (replace is default)
  //
  if ((orionldState.uriParamOptions.update == true) && (orionldState.uriParamOptions.replace == true))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "URI Param Error", "options: both /update/ and /replace/ present");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array with objects
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  // * no entity can contain an entity::type (one level down)
  //
  ARRAY_CHECK(orionldState.requestTree, "toplevel");
  EMPTY_ARRAY_CHECK(orionldState.requestTree, "toplevel");


  KjNode* incomingTree   = orionldState.requestTree;
  KjNode* successArrayP  = kjArray(orionldState.kjsonP, "success");
  KjNode* errorsArrayP   = kjArray(orionldState.kjsonP, "errors");

  //
  // Entities that already exist in the DB cannot have a type != type-in-db
  // To assure this, we need to extract all existing entities from the database
  // Also, those entities that do not exist MUST have an entity type present.
  //
  // Create idArray as an array of entity IDs, extracted from orionldState.requestTree
  //
  KjNode* idArray = kjEntityIdArrayExtract(orionldState.requestTree, successArrayP, errorsArrayP);

  //
  // 02. Query database extracting three fields: { id, type and creDate } for each of the entities
  //     whose Entity::Id is part of the array "idArray".
  //     The result is "idTypeAndCredateFromDb" - an array of "tiny" entities with { id, type and creDate }
  //
  KjNode* idTypeAndCreDateFromDb = dbEntityListLookupWithIdTypeCreDate(idArray);

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
      char*                  idInDb        = NULL;
      char*                  typeInDb      = NULL;
      double                 creDateInDb   = 0;
      char*                  typeInPayload = NULL;
      KjNode*                contextNodeP  = NULL;
      OrionldContext*        contextP      = NULL;
      KjNode*                entityP;
      OrionldProblemDetails  pd;

      // Get entity id, type and creDate from the DB
      entityTypeAndCreDateGet(dbEntityP, &idInDb, &typeInDb, &creDateInDb);

      //
      // For the entity in question - get id and type from the incoming payload
      // First look up the entity with ID 'idInDb' in the incoming payload
      //
      entityP = entityLookupById(incomingTree, idInDb);
      if (entityP == NULL)
        continue;

      typeInPayload = entityTypeGet(entityP, &contextNodeP);

      if (contextNodeP != NULL)
        contextP = orionldContextFromTree(NULL, true, contextNodeP, &pd);

      if (contextP == NULL)
        contextP = orionldState.contextP;

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
        char* typeInPayloadExpanded = orionldContextItemExpand(contextP, typeInPayload, true, NULL);

        if (strcmp(typeInPayloadExpanded, typeInDb) != 0)
        {
          //
          // As the entity type differed, this entity will not be updated in DB, nor will it be removed:
          // - removed from incomingTree
          // - not added to "removeArray"
          //
          LM_W(("Bad Input (orig entity type: '%s'. New entity type: '%s'", typeInDb, typeInPayloadExpanded));
          entityErrorPush(errorsArrayP, idInDb, OrionldBadRequestData, "non-matching entity type", typeInPayload, 400, false);
          kjChildRemove(incomingTree, entityP);
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
      KjNode* creDateNodeP = kjFloat(orionldState.kjsonP, idInDb, creDateInDb);
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
  if (orionldState.uriParamOptions.update == false)
  {
    if ((removeArray != NULL) && (removeArray->value.firstChildP != NULL))
      dbEntitiesDelete(removeArray);
  }


  //
  // 06. Fill in UpdateContextRequest from "incomingTree"
  //
  UpdateContextRequest  mongoRequest;
  KjNode*               cloneP = NULL;


  //
  // Before the tree is destroyed by kjTreeToUpdateContextRequest(),
  // remove+merge any duplicated entities from the array and save the array for temporal.
  //
  // If more than ONE instance of an entity:
  //   - For REPLACE - remove all entity instances but the last
  //   - For UPDATE  - remove all instances and add a new one - merged from all of them
  //
  duplicatedInstances(incomingTree, orionldState.uriParamOptions.update == false, errorsArrayP);

  if (temporal)
    cloneP = kjClone(orionldState.kjsonP, incomingTree);

  mongoRequest.updateActionType = ActionTypeAppend;

  kjTreeToUpdateContextRequest(&mongoRequest, incomingTree, errorsArrayP, idTypeAndCreDateFromDb);

  if (temporal)
    orionldState.requestTree = cloneP;

  //
  // 07. Set 'modDate' to "RIGHT NOW"
  //
  for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.size(); ++ix)
  {
    mongoRequest.contextElementVector[ix]->entityId.modDate = orionldState.timestamp.tv_sec;
  }


  //
  // 08. Call mongoBackend - to create/modify the entities
  //     In case of REPLACE, all entities have been removed from the DB prior to this call, so, they will all be created.
  //
  UpdateContextResponse mongoResponse;

  orionldState.httpStatusCode = mongoUpdateContext(&mongoRequest,
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

  if (orionldState.httpStatusCode == SccOk)
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
    {
      const char* entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

      if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == SccOk)
        entitySuccessPush(successArrayP, entityId);
      else
        entityErrorPush(errorsArrayP,
                        entityId,
                        OrionldBadRequestData,
                        "",
                        mongoResponse.contextElementResponseVector.vec[ix]->statusCode.reasonPhrase.c_str(),
                        400,
                        false);
    }

    for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.vec.size(); ix++)
    {
      const char* entityId = mongoRequest.contextElementVector.vec[ix]->entityId.id.c_str();

      if (kjStringValueLookupInArray(successArrayP, entityId) == NULL)
        entitySuccessPush(successArrayP, entityId);
    }

    //
    // Add the success/error arrays to the response-tree
    //
    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);

    orionldState.httpStatusCode = SccOk;
  }

  mongoRequest.release();
  mongoResponse.release();

  if (orionldState.httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext flagged an error"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Database Error");
    orionldState.httpStatusCode = SccReceiverInternalError;
    return false;
  }
  else if (errorsArrayP->value.firstChildP != NULL)  // There are entities in error
  {
    orionldState.httpStatusCode = SccMultiStatus;
    orionldState.noLinkHeader   = true;
  }
  else
  {
    orionldState.httpStatusCode = SccNoContent;
    orionldState.responseTree = NULL;
  }

  return true;
}
