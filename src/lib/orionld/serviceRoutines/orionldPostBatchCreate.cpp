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
#include "rest/uriParamNames.h"                                // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
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
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"        // pcheckEntityInfoArray
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"          // orionldUriExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjTreeToUpdateContextRequest.h"       // kjTreeToUpdateContextRequest
#include "orionld/serviceRoutines/orionldPostBatchCreate.h"    // Own Interface



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
    if (SCOMPARE3(nodeP->name, 'i', 'd', 0))
      *idP = nodeP->value.s;
  }
}


// ----------------------------------------------------------------------------
//
// orionldPostBatchCreate -
//
// POST /ngsi-ld/v1/entityOperations/create
//
// From the spec:
//   This operation allows creating a batch of NGSI-LD Entities, creating each of them if they don't exist.
//
bool orionldPostBatchCreate(ConnectionInfo* ciP)
{
  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  // * no entity can contain an entity::type (one level down)
  //
  pcheckEntityInfoArray(orionldState.requestTree, true);  // FIXME: This is not the correct check!

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
  KjNode* idTypeAndCreDateFromDb = dbEntityListLookupWithIdTypeCreDate(idArray);

  if (idTypeAndCreDateFromDb != NULL)
  {
    for (KjNode* dbEntityP = idTypeAndCreDateFromDb->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      char*    idInDb        = NULL;
      KjNode*  entityP;

      // Get entity id, type and creDate from the DB
      entityIdGet(dbEntityP, &idInDb);
      entityErrorPush(errorsArrayP, idInDb, OrionldBadRequestData, "entity already exists", NULL, 400);

      entityP = entityLookupById(incomingTree, idInDb);

      kjChildRemove(incomingTree, entityP);
    }
  }

  typeCheckForNonExistingEntities(incomingTree, idTypeAndCreDateFromDb, errorsArrayP, NULL);

  UpdateContextRequest  mongoRequest;

  mongoRequest.updateActionType = ActionTypeAppendStrict;

  kjTreeToUpdateContextRequest(&mongoRequest, incomingTree, errorsArrayP);

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

  return true;
}
