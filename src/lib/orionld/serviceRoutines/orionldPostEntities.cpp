/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjRender.h"                                      // kjRender
}

#include "common/globals.h"                                      // parse8601Time
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                           // orion::ValueType
#include "orionTypes/UpdateActionType.h"                         // ActionType
#include "parse/CompoundValueNode.h"                             // CompoundValueNode
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/rest/orionldServiceInit.h"                     // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldEntityPayloadCheck.h"            // orionldEntityPayloadCheck
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/mongoBackend/mongoEntityExists.h"              // mongoEntityExists
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(ConnectionInfo* ciP)
{
  OBJECT_CHECK(orionldState.requestTree, "toplevel");

  char*    detail;
  KjNode*  locationP          = NULL;
  KjNode*  observationSpaceP  = NULL;
  KjNode*  operationSpaceP    = NULL;
  KjNode*  createdAtP         = NULL;
  KjNode*  modifiedAtP        = NULL;

  if (orionldEntityPayloadCheck(orionldState.requestTree->value.firstChildP, &locationP, &observationSpaceP, &operationSpaceP, &createdAtP, &modifiedAtP, false) == false)
    return false;

  char*    entityId           = orionldState.payloadIdNode->value.s;
  char*    entityType         = orionldState.payloadTypeNode->value.s;


  //
  // Entity ID
  //
  if ((urlCheck(entityId, &detail) == false) && (urnCheck(entityId, &detail) == false))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity id", "The id specified cannot be resolved to a URL or URN");
    return false;
  }


  //
  // If the entity already exists, an error should be returned
  //
  if (mongoEntityExists(entityId, orionldState.tenant) == true)
  {
    orionldErrorResponseCreate(OrionldAlreadyExists, "Entity already exists", entityId);
    orionldState.httpStatusCode = SccConflict;
    return false;
  }

  orionldState.entityId = entityId;

  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;
  mongoRequest.updateActionType = ActionTypeAppend;

  entityIdP->id            = entityId;
  entityIdP->isPattern     = "false";
  entityIdP->creDate       = getCurrentTime();
  entityIdP->modDate       = getCurrentTime();
  entityIdP->isTypePattern = false;
  entityIdP->type          = orionldContextItemExpand(orionldState.contextP, entityType, NULL, true, NULL);


  //
  // Attributes
  //
  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtUriExpansion, ("treating entity node '%s'", kNodeP->name));

    if ((kNodeP == createdAtP) || (kNodeP == modifiedAtP))
      continue;

    ContextAttribute* caP            = new ContextAttribute();
    KjNode*           attrTypeNodeP  = NULL;
    char*             detail         = (char*) "none";

    if (kjTreeToContextAttribute(orionldState.contextP, kNodeP, caP, &attrTypeNodeP, &detail) == false)
    {
      // kjTreeToContextAttribute calls orionldErrorResponseCreate
      LM_E(("kjTreeToContextAttribute failed: %s", detail));
      delete caP;
      mongoRequest.release();
      return false;
    }

    if (attrTypeNodeP != NULL)
      ceP->contextAttributeVector.push_back(caP);
    else
      delete caP;
  }


  //
  // Mongo
  //
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

  mongoRequest.release();
  mongoResponse.release();

  if (orionldState.httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", orionldState.httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  orionldState.httpStatusCode = SccCreated;
  orionldState.entityCreated  = true;

  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", entityId);

  return true;
}
