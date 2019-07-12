/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccNotFound
#include "mongoBackend/mongoEntityExists.h"                      // mongoEntityExists
#include "mongoBackend/mongoAttributeExists.h"                   // mongoAttributeExists
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                    // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
#include "orionld/serviceRoutines/orionldPatchEntity.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPatchEntity -
//
bool orionldPatchEntity(ConnectionInfo* ciP)
{
  char* entityId = orionldState.wildcard[0];

  LM_T(LmtServiceRoutine, ("In orionldPatchEntity"));

  if (mongoEntityExists(entityId, orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Entity does not exist", entityId, OrionldDetailsString);
    return false;
  }

  // Is the payload empty?
  if (orionldState.requestTree == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload is missing", NULL, OrionldDetailsString);
    return false;
  }


  // Is the payload not a JSON object?
  if  (orionldState.requestTree->type != KjObject)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload is not a JSON object", kjValueType(orionldState.requestTree->type), OrionldDetailsString);
    return false;
  }

  // Is the payload an empty object?
  if  (orionldState.requestTree->value.firstChildP == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload is an empty JSON object", NULL, OrionldDetailsString);
    return false;
  }

  //
  // Make sure the attributes to be patched exist - FIXME: too damn slow to get an attribyte at a time - make a smarter query!
  //
  for (KjNode* attrNodeP = orionldState.requestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    char    longAttrName[256];
    char*   attrNameP;
    char*   details;

    if ((strncmp(attrNodeP->name, "http://", 7) == 0) || (strncmp(attrNodeP->name, "https://", 8) == 0))
      attrNameP = attrNodeP->name;
    else
    {
      // Get the long name of the Context Attribute name
      if (orionldUriExpand(orionldState.contextP, attrNodeP->name, longAttrName, sizeof(longAttrName), &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, details, attrNodeP->name, OrionldDetailsAttribute);
        return false;
      }
      attrNameP = longAttrName;
    }

    if (mongoAttributeExists(entityId, attrNameP, orionldState.tenant) == false)
    {
      ciP->httpStatusCode = SccNotFound;
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Attribute does not exist", attrNameP, OrionldDetailsString);
      return false;
    }
  }


  //
  // mongo ...
  // - fill in the UpdateContextRequest from the KjTree
  // - call the mongo routine 'mongoUpdateContext'
  //
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);

  entityIdP     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id = entityId;
  mongoRequest.updateActionType = ActionTypeAppendStrict;

  // Iterate over the object, to get all attributes
  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    KjNode*            attrTypeNodeP = NULL;
    ContextAttribute*  caP           = new ContextAttribute();

    // FIXME: Move attributeTreat to separate file
    extern bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP);
    if (attributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
    {
      mongoRequest.release();
      ciP->httpStatusCode = SccBadRequest;  // FIXME: Should be set inside 'attributeTreat' - could be 500, not 400 ...
      LM_E(("attributeTreat failed"));
      delete caP;
      return false;
    }

    //
    // URI Expansion for the attribute name, except if "location", "observationSpace", or "operationSpace"
    //
    if (SCOMPARE9(kNodeP->name,       'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
      caP->name = kNodeP->name;
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
      caP->name = kNodeP->name;
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
      caP->name = kNodeP->name;
    else
    {
      char  longName[256];
      char* details;

      if (orionldUriExpand(orionldState.contextP, kNodeP->name, longName, sizeof(longName), &details) == false)
      {
        delete caP;
        mongoRequest.release();
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, details, kNodeP->name, OrionldDetailsAttribute);
        return false;
      }

      caP->name = longName;
    }

    // NO URI Expansion for Attribute TYPE
    caP->type = attrTypeNodeP->value.s;

    // Add the attribute to the attr vector
    ceP->contextAttributeVector.push_back(caP);
  }

  //
  // Call mongoBackend - FIXME: call postUpdateContext, not mongoUpdateContext
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
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend", OrionldDetailsString);
    return false;
  }

  return true;
}
