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
* Author: Ken Zangelin, Gabriel Quaresma
*/
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kbase/kMacros.h"                                      // K_FT
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjBuilder.h"                                    // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                     // kjLookup
#include "kjson/kjClone.h"                                      // kjClone
}

#include "logMsg/logMsg.h"                                      // LM_*

#include "orionTypes/OrionValueType.h"                          // orion::ValueType
#include "orionTypes/UpdateActionType.h"                        // ActionType
#include "parse/CompoundValueNode.h"                            // CompoundValueNode
#include "ngsi/ContextAttribute.h"                              // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                        // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                       // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                    // mongoUpdateContext
#include "mongoBackend/MongoGlobal.h"                           // getMongoConnection()

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/common/SCOMPARE.h"                            // SCOMPAREx
#include "orionld/common/CHECK.h"                               // CHECK
#include "orionld/common/entitySuccessPush.h"                   // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                     // entityErrorPush
#include "orionld/common/entityLookupById.h"                    // entityLookupById
#include "orionld/common/removeArrayEntityLookup.h"             // removeArrayEntityLookup
#include "orionld/common/typeCheckForNonExistingEntities.h"     // typeCheckForNonExistingEntities
#include "orionld/common/duplicatedInstances.h"                 // duplicatedInstances
#include "orionld/common/performance.h"                         // PERFORMANCE
#include "orionld/service/orionldServiceInit.h"                 // orionldHostName, orionldHostNameLen
#include "orionld/context/orionldCoreContext.h"                 // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"              // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"      // orionldContextItemAliasLookup
#include "orionld/context/orionldContextFromTree.h"             // orionldContextFromTree
#include "orionld/kjTree/kjStringValueLookupInArray.h"          // kjStringValueLookupInArray
#include "orionld/legacyDriver/kjTreeToUpdateContextRequest.h"  // kjTreeToUpdateContextRequest
#include "orionld/kjTree/kjEntityIdArrayExtract.h"              // kjEntityIdArrayExtract
#include "orionld/kjTree/kjEntityArrayErrorPurge.h"             // kjEntityArrayErrorPurge
#include "orionld/payloadCheck/pCheckEntity.h"                  // pCheckEntity
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityListLookupWithIdTypeCreDate.h"   // mongoCppLegacyEntityListLookupWithIdTypeCreDate
#include "orionld/legacyDriver/legacyPostBatchUpdate.h"         // Own Interface



// -----------------------------------------------------------------------------
//
// entityTypeChange -
//
static bool entityTypeChange(KjNode* entityP, KjNode* dbEntityP, char** newTypeP)
{
  KjNode*  newTypeNodeP  = kjLookup(entityP, "type");
  char*    newType       = (newTypeNodeP != NULL)? newTypeNodeP->value.s : NULL;
  KjNode*  oldTypeNodeP  = kjLookup(dbEntityP, "type");
  char*    oldType       = (oldTypeNodeP != NULL)? oldTypeNodeP->value.s : NULL;

  if ((newType != NULL) && (oldType != NULL))
  {
    if (strcmp(newType, oldType) != 0)  // They differ
    {
      *newTypeP = newType;
      return true;
    }
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// legacyPostBatchUpdate -
//
// POST /ngsi-ld/v1/entityOperations/update
//
bool legacyPostBatchUpdate(void)
{
  // Error or not, the Link header should never be present in the reponse
  orionldState.noLinkHeader = true;

  // The response is never JSON-LD
  orionldState.out.contentType = MT_JSON;

  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array with objects
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  // * If entity::type is present, it must coincide with what's in the database
  //
  ARRAY_CHECK(orionldState.requestTree, "toplevel");
  EMPTY_ARRAY_CHECK(orionldState.requestTree, "toplevel");


  KjNode*  incomingTree   = orionldState.requestTree;
  KjNode*  successArrayP  = kjArray(orionldState.kjsonP, "success");
  KjNode*  errorsArrayP   = kjArray(orionldState.kjsonP, "errors");

  //
  // Entities that already exist in the DB cannot have a type != type-in-db
  // To assure this, we need to extract all existing entities from the database
  // Also, those entities that do not exist MUST have an entity type present.
  //
  // Create idArray as an array of entity IDs, extracted from orionldState.requestTree
  //
  KjNode* idArray = kjEntityIdArrayExtract(orionldState.requestTree, errorsArrayP);

  //
  // 02. Check whether some ID from idArray does not exist - that would be an error for Batch Update
  //     Check also that the entity type is the same, if given in the request
  //
  KjNode* idTypeAndCreDateFromDb = mongoCppLegacyEntityListLookupWithIdTypeCreDate(idArray, true);  // true: include attrNames array
  KjNode* entityP;

  if (idTypeAndCreDateFromDb == NULL)
  {
    // Nothing found in the DB - all entities marked as erroneous
    for (KjNode* idEntity = idArray->value.firstChildP; idEntity != NULL; idEntity = idEntity->next)
    {
      entityErrorPush(errorsArrayP, idEntity->value.s, OrionldBadRequestData, "entity does not exist", NULL, 400);
      entityP = entityLookupById(incomingTree, idEntity->value.s);
      kjChildRemove(incomingTree, entityP);
    }

    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);

    orionldState.out.contentType = MT_JSON;
    orionldState.httpStatusCode  = 207;

    return true;
  }


  OrionldContext* savedContext = orionldState.contextP;
  for (KjNode* entityP = incomingTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode*  idNodeP       = kjLookup(entityP, "id");
    char*    entityId;

    if (idNodeP == NULL)
      idNodeP = kjLookup(entityP, "@id");

    if (idNodeP == NULL)
    {
      entityErrorPush(errorsArrayP, "No entity id", OrionldBadRequestData, "entity without id", NULL, 400);
      continue;
    }

    if (idNodeP->type != KjString)
    {
      entityErrorPush(errorsArrayP, "Invalid entity id", OrionldBadRequestData, "entity::id must be a JSON String", kjValueType(idNodeP->type), 400);
      continue;
    }

    entityId = idNodeP->value.s;

    // Not existing entities cannot be updated
    KjNode* dbEntityP = entityLookupById(idTypeAndCreDateFromDb, entityId);
    if (dbEntityP == NULL)
    {
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "entity does not exist", NULL, 400);
      kjChildRemove(incomingTree, entityP);
      continue;
    }

    // If @context in payload body, it needs to be respected
    KjNode* contextNodeP  = kjLookup(entityP, "@context");
    if (contextNodeP != NULL)
      orionldState.contextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, contextNodeP);

    // Checking the entity and turning it Normalized
    if (pCheckEntity(entityP, true, NULL) == false)  // NULL ... I could give the DB Entity, just it's a REPLACE - not needed
    {
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, 400);
      kjChildRemove(incomingTree, entityP);
    }

    char* newType;
    if (entityTypeChange(entityP, dbEntityP, &newType) == true)
    {
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "non-matching entity type", newType, 400);
      kjChildRemove(incomingTree, entityP);
      continue;
    }
  }
  orionldState.contextP = savedContext;



  //
  // If already existing attributes are to be ignored (options=noOverwrite) -
  //   go over all entities and remove all attrs that already exist
  //
  if (orionldState.uriParamOptions.noOverwrite == true)
  {
    for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      KjNode* idNodeP   = kjLookup(entityP, "id");
      KjNode* dbEntityP = entityLookupById(idTypeAndCreDateFromDb, idNodeP->value.s);
      KjNode* attrNames = kjLookup(dbEntityP, "attrNames");

      //
      // Loop over the all the attributes of the entity in the incoming payload body
      // For each attribute - look up in dbEntity and if attribute found, remove it from the incoming payload body
      //
      KjNode* aP = entityP->value.firstChildP;
      KjNode* next;

      while (aP != NULL)
      {
        next = aP->next;

        if ((strcmp(aP->name, "id") != 0) && (strcmp(aP->name, "type") != 0))
        {
          if ((strcmp(aP->name, "createdAt")  == 0) || (strcmp(aP->name, "modifiedAt") == 0))
            kjChildRemove(entityP, aP);
          else if (kjStringValueLookupInArray(attrNames, aP->name) != NULL)
            kjChildRemove(entityP, aP);
        }

        aP = next;
      }
    }
  }


  //
  // Take care of duplicated instances of entities
  //
  if (orionldState.uriParamOptions.noOverwrite == true)
    duplicatedInstances(incomingTree, idTypeAndCreDateFromDb, false, false, errorsArrayP);  // attributeReplace == false => existing attrs are ignored
  else
    duplicatedInstances(incomingTree, NULL, false, true, errorsArrayP);                     // attributeReplace == true => existing attrs are replaced

  UpdateContextRequest  mongoRequest;
  KjNode*               treeP    = (troe == true)? kjClone(orionldState.kjsonP, incomingTree) : incomingTree;

  mongoRequest.updateActionType  = (orionldState.uriParamOptions.noOverwrite == true)? ActionTypeAppend : ActionTypeAppend;

  kjTreeToUpdateContextRequest(&mongoRequest, treeP, errorsArrayP, idTypeAndCreDateFromDb);


  //
  // 03. Set 'modDate' to "The time when the request entered"
  //
  for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.size(); ++ix)
  {
    mongoRequest.contextElementVector[ix]->entityId.modDate = orionldState.requestTime;
  }


  UpdateContextResponse    mongoResponse;
  std::vector<std::string> servicePathV;
  servicePathV.push_back("/");

  PERFORMANCE(mongoBackendStart);
  orionldState.httpStatusCode = mongoUpdateContext(&mongoRequest,
                                                   &mongoResponse,
                                                   orionldState.tenantP,
                                                   servicePathV,
                                                   orionldState.in.xAuthToken,
                                                   orionldState.correlator,
                                                   orionldState.attrsFormat,
                                                   orionldState.apiVersion,
                                                   NGSIV2_NO_FLAVOUR);
  PERFORMANCE(mongoBackendEnd);

  if (orionldState.httpStatusCode == 200)
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
    {
      const char* entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

      if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == 200)
        entitySuccessPush(successArrayP, entityId);
      else
      {
        LM_W(("mongoBackend Reports Error for entity '%s': %s",
              entityId,
              mongoResponse.contextElementResponseVector.vec[ix]->statusCode.details.c_str()));

        entityErrorPush(errorsArrayP,
                        entityId,
                        OrionldBadRequestData,
                        "MongoBackend Reports Error",
                        mongoResponse.contextElementResponseVector.vec[ix]->statusCode.reasonPhrase.c_str(),
                        mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code);
      }
    }

    //
    // Add the success/error arrays to the response-tree
    //
    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);

    orionldState.httpStatusCode = 200;
  }

  mongoRequest.release();
  mongoResponse.release();

  if (orionldState.httpStatusCode != 200)
  {
    orionldError(OrionldBadRequestData, "Internal Error", "Database Error", 500);
    return false;
  }
  else if (errorsArrayP->value.firstChildP != NULL)  // There are entities in error
  {
    orionldState.httpStatusCode  = 207;   // Multi-Status
    orionldState.out.contentType = MT_JSON;  // restReply already sets it to JSON is 207 ...
  }
  else
  {
    orionldState.httpStatusCode = 204;  // No Content
    orionldState.responseTree = NULL;
  }

  if (troe == true)
    kjEntityArrayErrorPurge(incomingTree, errorsArrayP, successArrayP);

  return true;
}
