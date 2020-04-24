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
* Author: Gabriel Quaresma
*/
extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjRender.h"                                    // kjRender
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
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityIdCheck.h"                      // entityIdCheck
#include "orionld/common/entityTypeCheck.h"                    // entityTypeCheck
#include "orionld/common/entityIdAndTypeGet.h"                 // entityIdAndTypeGet
#include "orionld/common/entityLookupById.h"                   // entityLookupById
#include "orionld/common/removeArrayEntityLookup.h"            // removeArrayEntityLookup
#include "orionld/common/typeCheckForNonExistingEntities.h"    // typeCheckForNonExistingEntities
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldContextFromTree.h"            // orionldContextFromTree
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"       // kjTreeToUpdateContextRequest
#include "orionld/serviceRoutines/orionldPostBatchUpdate.h"    // Own Interface



// ----------------------------------------------------------------------------
//
// entityIdPush - add ID to array
//
static void entityIdPush(KjNode* entityIdsArrayP, const char* entityId)
{
  KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, entityId);

  kjChildAdd(entityIdsArrayP, idNodeP);
}



// ----------------------------------------------------------------------------
//
// entitySuccessPush -
//
static void entitySuccessPush(KjNode* successArrayP, const char* entityId)
{
  KjNode* eIdP = kjString(orionldState.kjsonP, "id", entityId);

  kjChildAdd(successArrayP, eIdP);
}



// ----------------------------------------------------------------------------
//
// orionldPostEntityOperationsUpdate -
//
// POST /ngsi-ld/v1/entityOperations/update
//
bool orionldPostBatchUpdate(ConnectionInfo* ciP)
{
  KjNode*  incomingTree   = orionldState.requestTree;
  KjNode*  idArray        = kjArray(orionldState.kjsonP, NULL);
  KjNode*  successArrayP  = kjArray(orionldState.kjsonP, "success");
  KjNode*  errorsArrayP   = kjArray(orionldState.kjsonP, "errors");
  KjNode*  entityP;
  KjNode*  next;

  //
  // 01. Create idArray as an array of entity IDs, extracted from orionldState.requestTree
  //
  entityP = incomingTree->value.firstChildP;
  while (entityP)
  {
    next = entityP->next;

    char* entityId;
    char* entityType;

    //
    // entityIdAndTypeGet calls entityIdCheck/entityTypeCheck that adds the entity in errorsArrayP if needed
    //
    if (entityIdAndTypeGet(entityP, &entityId, &entityType, errorsArrayP) == true)
      entityIdPush(idArray, entityId);
    else
      kjChildRemove(incomingTree, entityP);

    entityP = next;
  }


  //
  // 02. Check whether some ID from idArray does not exist
  //     Check also that the entity type is the same, if given in the request
  //
  KjNode* idTypeAndCreDateFromDb = dbEntityListLookupWithIdTypeCreDate(idArray);

  if (idTypeAndCreDateFromDb == NULL)
  {
    // Nothing found in the DB - all entities marked as erroneous
    for (KjNode* idEntity = idArray->value.firstChildP; idEntity != NULL; idEntity = idEntity->next)
    {
      entityErrorPush(errorsArrayP, idEntity->value.s, OrionldBadRequestData, "entity does not exist", NULL, 400);
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

      if (entityP == NULL)
      {
        // This should never happen ...
        entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "entity seems to have disappeared from the incomingTree ... ???", NULL, 500);
        continue;
      }

      KjNode*  contextNodeP  = kjLookup(entityP, "@context");

      LM_TMP(("BUPD: @context of entity '%s': at %p", entityId, contextNodeP));
      if ((orionldState.ngsildContent == true) && (contextNodeP == NULL))
      {
        LM_W(("Bad Input (Content-Type == application/ld+json, but no @context in payload data array item)"));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid payload", "Content-Type is 'application/ld+json', but no @context in payload data array item", 400);
        kjChildRemove(incomingTree, entityP);
        continue;
      }
      else if ((orionldState.ngsildContent == false) && (contextNodeP != NULL))
      {
        LM_W(("Bad Input (Content-Type is 'application/json', and an @context is present in the payload data array item)"));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid payload", "Content-Type is 'application/json', and an @context is present in the payload data array item", 400);
        kjChildRemove(incomingTree, entityP);
        continue;
      }
      else if ((contextNodeP != NULL) && (orionldState.linkHttpHeaderPresent == true))
      {
        LM_W(("Bad Input (@context present both in Link header and in payload data)"));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Inconsistency between HTTP headers and payload data", "@context present both in Link header and in payload data", 400);
        kjChildRemove(incomingTree, entityP);
        continue;
      }


      OrionldContext*        contextP;
      OrionldProblemDetails  pd;

      if (contextNodeP != NULL)
      {
        LM_TMP(("BUPD: Creating the @context from the tree"));
        contextP = orionldContextFromTree(NULL, false, contextNodeP, &pd);
      }
      else
        contextP = orionldCoreContextP;

      KjNode*  dbEntityP = entityLookupById(idTypeAndCreDateFromDb, entityId);

      if (dbEntityP == NULL)
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "entity does not exist", NULL, 400);
        kjChildRemove(incomingTree, entityP);
        continue;
      }


      KjNode*  inTypeP = kjLookup(entityP, "type");

      if (inTypeP != NULL)  // Make sure it's a string and compare with what's in the DB
      {
        if (inTypeP->type != KjString)
        {
          entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "entity type is not a JSON string", kjValueType(inTypeP->type), 400);
          kjChildRemove(incomingTree, entityP);
          continue;
        }

        //
        // Compare the value with what's in the DB
        //
        KjNode* dbTypeP = kjLookup(dbEntityP, "type");

        if (dbTypeP == NULL)
        {
          entityErrorPush(errorsArrayP, entityId, OrionldInternalError, "no entity type in DB", NULL, 500);
          kjChildRemove(incomingTree, entityP);
          continue;
        }

        char* expandedType = orionldContextItemExpand(contextP, inTypeP->value.s, NULL, true, NULL);
        if (strcmp(expandedType, dbTypeP->value.s) != 0)
        {
          LM_TMP(("BUPD: inTypeP->value.s: '%s'", inTypeP->value.s));
          LM_TMP(("BUPD: idbTypeP->value.s: '%s'", dbTypeP->value.s));
          entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "non-matching entity type", inTypeP->value.s, 400);
          kjChildRemove(incomingTree, entityP);
          continue;
        }
      }
    }
  }

  UpdateContextRequest  mongoRequest;

  mongoRequest.updateActionType = ActionTypeUpdate;

  kjTreeToUpdateContextRequest(&mongoRequest, incomingTree, errorsArrayP);

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

  return true;
}
