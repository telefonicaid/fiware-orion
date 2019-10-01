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
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                    // orionldState
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPatchAttribute -
//
bool orionldPatchAttribute(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldPatchAttribute"));

  if (mongoEntityExists(orionldState.wildcard[0], orionldState.tenant) == false)
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

  // Make sure the attribute to be patched exists
  if (mongoAttributeExists(orionldState.wildcard[0], orionldState.wildcard[1], orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute does not exist", orionldState.wildcard[1]);
    return false;
  }

  ciP->httpStatusCode = SccNotImplemented;
  orionldErrorResponseCreate(OrionldBadRequestData, "Not implemented - PATCH /ngsi-ld/v1/entities/*/attrs", orionldState.wildcard[0]);
  return true;
}
