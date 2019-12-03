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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

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
#include "orionld/common/orionldAttributeTreat.h"                // orionldAttributeTreat
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // orionldContextValueExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldPatchEntity.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPatchEntity -
//
bool orionldPatchEntity(ConnectionInfo* ciP)
{
  char*   entityId          = orionldState.wildcard[0];
  KjNode* currentEntityTree;

  LM_T(LmtServiceRoutine, ("In orionldPatchEntity"));

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
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is not a JSON object", kjValueType(orionldState.requestTree->type));
    return false;
  }

  // Is the payload an empty object?
  if  (orionldState.requestTree->value.firstChildP == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is an empty JSON object", NULL);
    return false;
  }

  if ((currentEntityTree = dbEntityLookup(entityId)) == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", entityId);
    return false;
  }

  //
  // o Make sure the attributes to be patched exist
  // o Expand attribute values if the @context says they should be expanded
  //
  KjNode* attrNamesArrayP = kjLookup(currentEntityTree, "attrNames");

  if (attrNamesArrayP == NULL)
    LM_X(1, ("Something has really gone wrong - the node 'attrNames' of the entity '%s' isn't present!", entityId));

  for (KjNode* attrNodeP = orionldState.requestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    char* attrNameP;

    if ((strncmp(attrNodeP->name, "http://", 7) == 0) || (strncmp(attrNodeP->name, "https://", 8) == 0))
      attrNameP = attrNodeP->name;
    else
    {
      bool valueToBeExpanded = false;

      attrNameP = orionldContextItemExpand(orionldState.contextP, attrNodeP->name, &valueToBeExpanded, true, NULL);

      if (valueToBeExpanded == true)
        orionldContextValueExpand(attrNodeP);
    }

    if (kjStringValueLookupInArray(attrNamesArrayP, attrNameP) == NULL)
    {
      ciP->httpStatusCode = SccNotFound;
      orionldErrorResponseCreate(OrionldBadRequestData, "Attribute does not exist", attrNameP);
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
  ContextElement         ce;
  EntityId*              entityIdP;
  ContextAttribute*      delayedDelete[20];  // If more attributes, then ... :(

  mongoRequest.contextElementVector.push_back(&ce);

  entityIdP     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id = entityId;
  mongoRequest.updateActionType = ActionTypeAppendStrict;

  // Iterate over the object, to get all attributes
  unsigned int attrs = 0;

  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (strcmp(kNodeP->name, "createdAt") == 0)
      continue;

    if (strcmp(kNodeP->name, "modifiedAt") == 0)
      continue;

    KjNode*            attrTypeNodeP = NULL;
    ContextAttribute*  caP           = new ContextAttribute();  // I have tried to use a stack variable "ContextAttribute ca" instead of allocating
                                                                // but I get problems with it. Need a delayed delete for this.
    delayedDelete[attrs]     = caP;
    delayedDelete[attrs + 1] = NULL;
    ++attrs;

    if (attrs >= sizeof(delayedDelete) / sizeof(delayedDelete[0]))
      LM_X(1, ("Need a bigger vector for delayed deletion of attributes"));

    char* detail;
    if (orionldAttributeTreat(ciP, kNodeP, caP, &attrTypeNodeP, &detail) == false)
    {
      mongoRequest.release();
      LM_E(("orionldAttributeTreat failed: %s", detail));
      delete caP;
      return false;
    }

    if (attrTypeNodeP != NULL)
      ce.contextAttributeVector.push_back(caP);
    else
      delete caP;
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

  //
  // Delay-Delete allocated attrs
  //
  for (unsigned int ix = 0; ix < attrs; ix++)
    delete delayedDelete[ix];

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
