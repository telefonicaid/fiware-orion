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

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccNotFound
#include "ngsi/ContextElement.h"                                 // ContextElement

#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "mongoBackend/mongoQueryContext.h"                      // mongoQueryContext

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"       // kjTreeFromQueryContextResponse
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/context/orionldContextItemExpand.h"            // orionldUriExpand
#include "orionld/mongoBackend/mongoAttributeExists.h"           // mongoAttributeExists
#include "orionld/mongoBackend/mongoEntityExists.h"              // mongoEntityExists
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPatchAttribute -
//
bool orionldPatchAttribute(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldPatchAttribute"));

  char* entityId = orionldState.wildcard[0];

  if (mongoEntityExists(entityId, orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0]);
    return false;
  }

  // Is the payload empty?
  if (orionldState.requestTree == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is missing", NULL);
    return false;
  }

  // Is the payload not a JSON object?
  if  (orionldState.requestTree->type != KjObject)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload not a JSON object", kjValueType(orionldState.requestTree->type));
    return false;
  }

  // Is the payload an empty object?
  if  (orionldState.requestTree->value.firstChildP == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is an empty JSON object", NULL);
    return false;
  }

  // validate the payload content informed by user e get the attribute value
  KjValue        attrValue;
  KjValueType    attrValueType = KjNone;

  for (KjNode* userPayloadNodeP = orionldState.requestTree->value.firstChildP; userPayloadNodeP != NULL; userPayloadNodeP = userPayloadNodeP->next)
  {
    if (SCOMPARE9(userPayloadNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      // Do Nothing
    }
    else if (SCOMPARE6(userPayloadNodeP->name, 'v', 'a', 'l', 'u', 'e', 0))  // the updated attr is a property or geoproperty
    {
      // Get the attribute value and the attribute value type
      attrValue     = userPayloadNodeP->value;
      attrValueType = userPayloadNodeP->type;
    }
    else if (SCOMPARE7(userPayloadNodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))  // the updated attr is a relationship
    {
      // Get the attribute object and the attrbibue value type
      attrValue     = userPayloadNodeP->value;
      attrValueType = userPayloadNodeP->type;
    }
    else
    {
      // Invalid key in payload
      ciP->httpStatusCode = SccBadRequest;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid key in the payload:", userPayloadNodeP->name);
      return false;
    }
  }

  //
  // URI Expansion for the attribute name, except if "location", "observationSpace", or "operationSpace"
  //
  char*  attrName;

  if (SCOMPARE9(orionldState.wildcard[1],       'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    attrName = orionldState.wildcard[1];
  else if (SCOMPARE17(orionldState.wildcard[1], 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    attrName = orionldState.wildcard[1];
  else if (SCOMPARE15(orionldState.wildcard[1], 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    attrName = orionldState.wildcard[1];
  else
  {
    attrName = orionldContextItemExpand(orionldState.contextP, orionldState.wildcard[1], NULL, true, NULL);
    if (attrName == NULL)
    {
     orionldErrorResponseCreate(OrionldBadRequestData, "error expanding attribute name", orionldState.wildcard[1]);
     return false;
    }
  }


  // Make sure the attribute to be patched exists
  if (mongoAttributeExists(entityId, attrName, orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute does not exist", orionldState.wildcard[1]);
    return false;
  }


  bool                  keyValues = false;
  QueryContextRequest   request;
  QueryContextResponse  response;
  EntityId              updatedEntityId(orionldState.wildcard[0], "", "false", false);
  char*                 details;

  request.entityIdVector.push_back(&updatedEntityId);

  //
  // Make sure the ID (orionldState.wildcard[0]) is a valid URI
  //
  if ((urlCheck(orionldState.wildcard[0], &details) == false) && (urnCheck(orionldState.wildcard[0], &details) == false))
  {
    LM_W(("Bad Input (Invalid Entity ID - Not a URL nor a URN)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN");
    return false;
  }


  // retrieve the updated entity from database
  ciP->httpStatusCode = mongoQueryContext(&request,
                                          &response,
                                          orionldState.tenant,
                                          ciP->servicePathV,
                                          ciP->uriParam,
                                          ciP->uriParamOptions,
                                          NULL,
                                          ciP->apiVersion);

  if (response.errorCode.code == SccBadRequest)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Bad Request", NULL);
    return false;
  }

  KjNode* myEntity     = kjTreeFromQueryContextResponse(ciP, true, NULL , keyValues, &response);
  KjNode* updatedAttrP = NULL;

  if (myEntity == NULL)
    ciP->httpStatusCode = SccContextElementNotFound;
  else
  {
    // update the attribute value based in content informed by user
    for (KjNode* kNodeP = myEntity->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
    {
      if (strcmp(kNodeP->name, orionldState.wildcard[1]) == 0)
      {
        updatedAttrP = kNodeP;

        for (KjNode* attrP = updatedAttrP->value.firstChildP; attrP != NULL; attrP = attrP->next)
        {
          if (SCOMPARE6(attrP->name, 'v',  'a', 'l', 'u', 'e', 0))  // update the property or geoproperty value
          {
            attrP->value = attrValue;
            attrP->type  = attrValueType;
            break;
          }
          else if (SCOMPARE7(attrP->name, 'o',  'b', 'j', 'e', 'c', 't', 0))  // update the relationship value
          {
            attrP->value = attrValue;
            attrP->type  = attrValueType;
            break;
          }
        }

        break;
      }
    }
  }

  // Store the new attribute value in the database
  KjNode*                attrTypeNodeP = NULL;
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);

  entityIdP                     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id                 = entityId;
  mongoRequest.updateActionType = ActionTypeAppendStrict;

  ContextAttribute*  caP           = new ContextAttribute();
  char*              detail;

  if (kjTreeToContextAttribute(ciP, updatedAttrP, caP, &attrTypeNodeP, &detail) == false)
  {
    mongoRequest.release();
    delete caP;
    return false;
  }

  if (attrTypeNodeP != NULL)
    ceP->contextAttributeVector.push_back(caP);
  else
    delete caP;

  //
  // Call mongoBackend
  //
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
  if (ciP->httpStatusCode == SccOk)
    ciP->httpStatusCode = SccNoContent;
  else
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  return true;
}
