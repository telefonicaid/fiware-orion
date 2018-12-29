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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotFound
#include "mongoBackend/mongoEntityExists.h"                    // mongoEntityExists
#include "mongoBackend/mongoAttributeExists.h"                 // mongoAttributeExists
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldPatchAttribute.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPatchAttribute -
//
bool orionldPatchAttribute(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldPatchAttribute"));

  if (mongoEntityExists(ciP->wildcard[0], ciP->tenant.c_str()) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Entity does not exist", ciP->wildcard[0], OrionldDetailsString);
    return false;
  }

  // Is the payload empty?
  if (ciP->requestTree == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No payload", NULL, OrionldDetailsString);
    return false;
  }

  // Is the payload not a JSON object?
  if  (ciP->requestTree->type != KjObject)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload not a JSON object", kjValueType(ciP->requestTree->type), OrionldDetailsString);
    return false;
  }

  // Is the payload an empty object?
  if  (ciP->requestTree->children == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload is an empty JSON object", NULL, OrionldDetailsString);
    return false;
  }

  // Make sure the attribute to be patched exists
  if (mongoAttributeExists(ciP->wildcard[0], ciP->wildcard[1], ciP->tenant.c_str()) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Attribute does not exist", ciP->wildcard[1], OrionldDetailsString);
    return false;
  }

  ciP->httpStatusCode = SccNotImplemented;
  orionldErrorResponseCreate(ciP, OrionldBadRequestData, "not implemented - PATCH /ngsi-ld/v1/entities/*/attrs", ciP->wildcard[0], OrionldDetailsString);
  return true;
}
