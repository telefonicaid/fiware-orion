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
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection()

#include "orionld/rest/orionldServiceInit.h"                   // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECK
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityLookupById.h"                   // entityLookupById
#include "orionld/common/removeArrayEntityLookup.h"            // removeArrayEntityLookup
#include "orionld/common/typeCheckForNonExistingEntities.h"    // typeCheckForNonExistingEntities
#include "orionld/common/duplicatedInstances.h"                // duplicatedInstances
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldContextFromTree.h"            // orionldContextFromTree
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"       // kjTreeToUpdateContextRequest
#include "orionld/kjTree/kjEntityIdArrayExtract.h"             // kjEntityIdArrayExtract
#include "orionld/kjTree/kjEntityArrayErrorPurge.h"            // kjEntityArrayErrorPurge
#include "orionld/serviceRoutines/orionldPostBatchUpdate.h"    // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpdate -
//
// POST /ngsi-ld/v1/entityOperations/update
//
bool orionldPostBatchUpdate(ConnectionInfo* ciP)
{
  // Error or not, the Link header should never be present in the reponse
  orionldState.noLinkHeader = true;

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
  KjNode* idArray = kjEntityIdArrayExtract(orionldState.requestTree, successArrayP, errorsArrayP);

  //
  // 02. Check whether some ID from idArray does not exist - that would be an error for Batch Update
  //     Check also that the entity type is the same, if given in the request
  //
  KjNode* idTypeAndCreDateFromDb = dbEntityListLookupWithIdTypeCreDate(idArray, true);  // true: include attrNames array
  KjNode* entityP;

  if (idTypeAndCreDateFromDb == NULL)
  {
    // Nothing found in the DB - all entities marked as erroneous
    for (KjNode* idEntity = idArray->value.firstChildP; idEntity != NULL; idEntity = idEntity->next)
    {
      entityErrorPush(errorsArrayP, idEntity->value.s, OrionldBadRequestData, "entity does not exist", NULL, 400, true);
      entityP = entityLookupById(incomingTree, idEntity->value.s);
      kjChildRemove(incomingTree, entityP);
    }
  }
  else
  {
    for (KjNode* idEntity = idArray->value.firstChildP; idEntity != NULL; idEntity = idEntity->next)
    {
      char*    entityId      = idEntity->value.s;
      KjNode*  entityP       = entityLookupById(incomingTree, entityId);

      if (entityP == NULL)  // This should never happen ...
      {
        LM_E(("Internal Error (Unable to find entity '%s')", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "entity disappeared from incomingTree", NULL, 500, false);
        continue;
      }

      KjNode*                contextNodeP  = kjLookup(entityP, "@context");
      OrionldProblemDetails  pd;


      KjNode*  dbEntityP = entityLookupById(idTypeAndCreDateFromDb, entityId);

      if (dbEntityP == NULL)
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "entity does not exist", NULL, 400, true);
        kjChildRemove(incomingTree, entityP);
        continue;
      }

      KjNode*  inTypeP = kjLookup(entityP, "type");

      if (inTypeP != NULL)  // Make sure it's a string and compare with what's in the DB
      {
        if (inTypeP->type != KjString)
        {
          LM_W(("Bad Input (Entity type is not a JSON string)"));
          entityErrorPush(errorsArrayP,
                          entityId,
                          OrionldInternalError,
                          "entity type is not a JSON string",
                          kjValueType(inTypeP->type),
                          400,
                          false);
          kjChildRemove(incomingTree, entityP);
          continue;
        }

        //
        // Compare the value with what's in the DB
        //
        KjNode* dbTypeP = kjLookup(dbEntityP, "type");

        if (dbTypeP == NULL)
        {
          LM_E(("Internal Error (no entity type in DB)"));
          entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "no entity type in DB", NULL, 500, true);
          kjChildRemove(incomingTree, entityP);
          continue;
        }

        OrionldContext* contextP;

        if      (contextNodeP          != NULL)  contextP = orionldContextFromTree(NULL, false, contextNodeP, &pd);
        else if (orionldState.contextP != NULL)  contextP = orionldState.contextP;
        else                                     contextP = orionldCoreContextP;

        char* expandedType = orionldContextItemExpand(contextP, inTypeP->value.s, true, NULL);
        if (strcmp(expandedType, dbTypeP->value.s) != 0)
        {
          LM_W(("Bad Input (non-matching entity type: '%s' vs '%s')", expandedType, dbTypeP->value.s));
          entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "non-matching entity type", inTypeP->value.s, 400, false);
          kjChildRemove(incomingTree, entityP);
          continue;
        }
      }
    }
  }


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
          char* longName = aP->name;

          if ((strcmp(aP->name, "location")         == 0) ||
              (strcmp(aP->name, "observationSpace") == 0) ||
              (strcmp(aP->name, "operationSpace")   == 0))
          {
          }
          else if ((strcmp(aP->name, "createdAt")  == 0) ||
                   (strcmp(aP->name, "modifiedAt") == 0))
          {
            kjChildRemove(entityP, aP);
            aP = next;
            continue;
          }
          else
            longName = orionldContextItemExpand(orionldState.contextP, aP->name, true, NULL);

          if (kjStringValueLookupInArray(attrNames, longName) != NULL)
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
  // 03. Set 'modDate' to "RIGHT NOW"
  //
  time_t now = time(NULL);

  for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.size(); ++ix)
  {
    mongoRequest.contextElementVector[ix]->entityId.modDate = now;
  }


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

  if (orionldState.httpStatusCode == SccOk)
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
    {
      const char* entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

      if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == SccOk)
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
                        mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code,
                        false);
      }
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
    orionldState.httpStatusCode = SccMultiStatus;
  else
  {
    orionldState.httpStatusCode = SccNoContent;
    orionldState.responseTree = NULL;
  }

  if (troe == true)
    kjEntityArrayErrorPurge(incomingTree, errorsArrayP, successArrayP);

  return true;
}
