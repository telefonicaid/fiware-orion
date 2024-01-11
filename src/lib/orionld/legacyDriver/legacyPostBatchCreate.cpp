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
#include <string>                                                // std::string
#include <vector>                                                // std::vector

extern "C"
{
#include "kbase/kMacros.h"                                       // K_FT
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjRender.h"                                      // kjRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionTypes/OrionValueType.h"                           // orion::ValueType
#include "orionTypes/UpdateActionType.h"                         // ActionType
#include "parse/CompoundValueNode.h"                             // CompoundValueNode
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection()

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // ARRAY_CHECK
#include "orionld/common/entityErrorPush.h"                      // entityErrorPush
#include "orionld/common/entityIdCheck.h"                        // entityIdCheck
#include "orionld/common/entityTypeCheck.h"                      // entityTypeCheck
#include "orionld/common/entityIdAndTypeGet.h"                   // entityIdAndTypeGet
#include "orionld/common/entityLookupById.h"                     // entityLookupById
#include "orionld/common/removeArrayEntityLookup.h"              // removeArrayEntityLookup
#include "orionld/common/typeCheckForNonExistingEntities.h"      // typeCheckForNonExistingEntities
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/service/orionldServiceInit.h"                  // orionldHostName, orionldHostNameLen
#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"               // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"            // orionldUriExpand
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/legacyDriver/kjTreeToUpdateContextRequest.h"   // kjTreeToUpdateContextRequest
#include "orionld/kjTree/kjEntityArrayErrorPurge.h"              // kjEntityArrayErrorPurge
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityListLookupWithIdTypeCreDate.h"   // mongoCppLegacyEntityListLookupWithIdTypeCreDate
#include "orionld/legacyDriver/legacyPostBatchCreate.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// entitySuccessPush -
//
static void entitySuccessPush(KjNode* successArrayP, const char* entityId)
{
  KjNode* eIdP = kjString(orionldState.kjsonP, NULL, entityId);

  kjChildAdd(successArrayP, eIdP);
}



// ----------------------------------------------------------------------------
//
// entityIdPush - add ID to array
//
static void entityIdPush(KjNode* entityIdsArrayP, const char* entityId)
{
  KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, entityId);

  kjChildAdd(entityIdsArrayP, idNodeP);
}



// -----------------------------------------------------------------------------
//
// entityIdGet -
//
static void entityIdGet(KjNode* dbEntityP, char** idP)
{
  for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE3(nodeP->name, 'i', 'd', 0) || SCOMPARE4(nodeP->name, '@', 'i', 'd', 0))
      *idP = nodeP->value.s;
  }
}



// -----------------------------------------------------------------------------
//
// idArrayGet -
//
static KjNode* idArrayGet(KjNode* treeP, KjNode* errorsArrayP)
{
  KjNode* idArray = kjArray(orionldState.kjsonP, NULL);
  KjNode* entityP = treeP->value.firstChildP;
  KjNode* next;

  while (entityP)
  {
    next = entityP->next;

    char* entityId;
    char* entityType;

    // entityIdAndTypeGet calls entityIdCheck/entityTypeCheck that adds the entity in errorsArrayP if needed
    if (entityIdAndTypeGet(entityP, &entityId, &entityType, errorsArrayP) == true)
      entityIdPush(idArray, entityId);
    else
      kjChildRemove(treeP, entityP);

    entityP = next;
  }

  return idArray;
}



// ----------------------------------------------------------------------------
//
// legacyPostBatchCreate -
//
// POST /ngsi-ld/v1/entityOperations/create
//
// From the spec:
//   This operation allows creating a batch of NGSI-LD Entities, creating each of them if they don't exist.
//   Error or not, the Link header should never be present in the reponse
//   And, the response is never JSON-LD
//
//
// Prerequisites for the payload in orionldState.requestTree:
// * must be an array
// * cannot be empty
// * all entities must contain an entity::id (one level down)
// * no entity can contain an entity::type (one level down)
//
bool legacyPostBatchCreate(void)
{
  orionldState.noLinkHeader    = true;
  orionldState.out.contentType = MT_JSON;

  ARRAY_CHECK(orionldState.requestTree, "incoming payload body");
  EMPTY_ARRAY_CHECK(orionldState.requestTree, "incoming payload body");

  KjNode*               incomingTree   = orionldState.requestTree;
  KjNode*               successArrayP  = kjArray(orionldState.kjsonP, "success");
  KjNode*               errorsArrayP   = kjArray(orionldState.kjsonP, "errors");
  //
  // 01. Create idArray as an array of entity IDs, extracted from orionldState.requestTree
  //
  KjNode* idArray = idArrayGet(orionldState.requestTree, errorsArrayP);


  //
  // 02. Query database extracting three fields: { id, type and creDate } for each of the entities
  //     whose Entity::Id is part of the array "idArray".
  //     The result is "idTypeAndCredateFromDb" - an array of "tiny" entities with { id, type, creDate }
  //
  // This is a CREATE operation, so, those entities that already exist ... give error
  //
  KjNode* idTypeAndCreDateFromDb = mongoCppLegacyEntityListLookupWithIdTypeCreDate(idArray, false);

  if (idTypeAndCreDateFromDb != NULL)
  {
    for (KjNode* dbEntityP = idTypeAndCreDateFromDb->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      char*    idInDb = NULL;
      KjNode*  entityP;

      // Get entity id, type and creDate from the DB
      entityIdGet(dbEntityP, &idInDb);
      entityErrorPush(errorsArrayP, idInDb, OrionldBadRequestData, "entity already exists", NULL, 400);

      entityP = entityLookupById(incomingTree, idInDb);

      kjChildRemove(incomingTree, entityP);
    }
  }

  typeCheckForNonExistingEntities(incomingTree, idTypeAndCreDateFromDb, errorsArrayP, NULL);


  //
  // Attempts to create an entity more than once (more than one instance with the same Entity ID in the entity array)
  // shall result in an error message (part of 207 response) for all except the first instance (which shall work just fine)
  //
  KjNode*          entityP       = orionldState.requestTree->value.firstChildP;
  KjNode*          next;
  OrionldContext*  savedContextP = orionldState.contextP;

  while (entityP != NULL)
  {
    next = entityP->next;

    //
    // Get the 'id' field
    //
    KjNode* idP = kjLookup(entityP, "id");
    if (idP == NULL)
    {
      idP = kjLookup(entityP, "@id");
      if (idP != NULL)
        idP->name = (char*) "id";
    }

    if (idP == NULL)
    {
      LM_E(("Internal Error (no 'id' for entity in batch create entity array - how did this get all the way here?)"));
      entityP = next;
      continue;
    }

    //
    // Compare the 'id' field of current (entityP) with all nextcoming EIDs is the array
    // If match, remove the latter
    //
    char*   entityId = idP->value.s;
    KjNode* copyP    = entityP->next;
    KjNode* copyNext;

    while (copyP != NULL)
    {
      copyNext = copyP->next;

      // Lookup the 'id' field
      KjNode* copyIdP = kjLookup(copyP, "id");
      if (copyIdP == NULL)
      {
        LM_E(("Internal Error (no 'id' for entity in batch create entity array - how did this get all the way here?)"));
        copyP = copyNext;
        continue;
      }

      if (strcmp(entityId, copyIdP->value.s) == 0)
      {
        entityErrorPush(errorsArrayP, copyIdP->value.s, OrionldBadRequestData, "Entity ID repetition", NULL, 400);
        kjChildRemove(orionldState.requestTree, copyP);
      }
      copyP = copyNext;
    }

    KjNode*                contextNodeP = kjLookup(entityP, "@context");
    OrionldContext*        contextP     = NULL;

    if (contextNodeP != NULL)
      contextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, contextNodeP);

    if (contextP != NULL)
      orionldState.contextP = contextP;

    // Entity ok, from a "repetition point of view", now, let's MAKE SURE it's correct!
    if (pCheckEntity(entityP, true, NULL) == false)
    {
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, 400);
      kjChildRemove(orionldState.requestTree, entityP);
    }

    entityP = next;
  }
  orionldState.contextP = savedContextP;

  //
  // Now that:
  //   - the erroneous entities have been removed from the incoming tree,
  //   - entities that already existed have been removed from the incoming tree,
  // Let's clone the tree for TRoE
  //
  KjNode* cloneP = NULL;  // Only for TRoE
  if (troe)
    cloneP = kjClone(orionldState.kjsonP, orionldState.requestTree);

  UpdateContextRequest  mongoRequest;

  mongoRequest.updateActionType = ActionTypeAppendStrict;

  kjTreeToUpdateContextRequest(&mongoRequest, incomingTree, errorsArrayP, NULL);

  //
  // DB update - if there is anything to update
  //
  orionldState.noDbUpdate = mongoRequest.contextElementVector.size() <= 0;
  if (orionldState.noDbUpdate == false)
  {
    UpdateContextResponse    mongoResponse;
    std::vector<std::string> servicePathV;
    servicePathV.push_back("/");

    orionldState.httpStatusCode = mongoUpdateContext(&mongoRequest,
                                                     &mongoResponse,
                                                     orionldState.tenantP,
                                                     servicePathV,
                                                     orionldState.in.xAuthToken,
                                                     orionldState.correlator,
                                                     orionldState.attrsFormat,
                                                     orionldState.apiVersion,
                                                     NGSIV2_NO_FLAVOUR);

    if (orionldState.httpStatusCode == 200)
    {
      // orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

      for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
      {
        const char* entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

        if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == 200)
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
        const char* entityId = mongoRequest.contextElementVector.vec[ix]->entityId.id.c_str();

        if (kjStringValueLookupInArray(successArrayP, entityId) == NULL)
          entitySuccessPush(successArrayP, entityId);
      }
    }
    else
    {
      orionldState.noDbUpdate = true;
      LM_E(("Database Error (mongoUpdateContext returned %d (!200))", orionldState.httpStatusCode));
    }

    mongoRequest.release();
    mongoResponse.release();

    if (orionldState.httpStatusCode != 200)
    {
      orionldError(OrionldBadRequestData, "Internal Error", "Database Error", 500);
      return false;
    }
  }


  //
  // Add the success/error arrays to the response-tree
  //
  if (errorsArrayP->value.firstChildP == NULL)  // No errors - 201 and String[] as payload body
  {
    orionldState.httpStatusCode  = 201;
    orionldState.responseTree    = successArrayP;
    orionldState.out.contentType = MT_JSON;
  }
  else
  {
    orionldState.httpStatusCode  = 207;
    orionldState.responseTree    = kjObject(orionldState.kjsonP, NULL);
    orionldState.out.contentType = MT_JSON;

    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);
  }

  if ((orionldState.noDbUpdate == false) && (orionldState.tenantP != &tenant0))
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);

  if ((troe == true) && (cloneP != NULL))
  {
    if (orionldState.noDbUpdate == false)
    {
      orionldState.requestTree = cloneP;
      kjEntityArrayErrorPurge(orionldState.requestTree, errorsArrayP, successArrayP);
    }
  }

  return true;
}
