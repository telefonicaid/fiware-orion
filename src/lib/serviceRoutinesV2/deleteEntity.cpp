/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/OrionError.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/deleteEntity.h"
#include "serviceRoutines/postUpdateContext.h"
#include "parse/forbiddenChars.h"



/* ****************************************************************************
*
* deleteEntity -
*
* DELETE /v2/entities/{entityId}
*
* Payload In:  None
* Payload Out: None
*
* URI parameters:
*   -
*/

using std::string;
using std::vector;

std::string deleteEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity* eP;

  if (forbiddenIdChars(ciP->apiVersion, compV[2].c_str() , NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  eP       = new Entity();
  eP->id   = compV[2];
  eP->type = ciP->uriParam["type"];

  if (compV.size() == 5)  // Deleting an attribute
  {
    ContextAttribute* caP = new ContextAttribute;
    caP->name = compV[4];
    eP->attributeVector.push_back(caP);
  }

  // Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, ActionTypeDelete);

  // Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);

  ciP->outMimeType = JSON;

  // Check for potential error
  string  answer = "";
  if (parseDataP->upcrs.res.oe.code != SccNone)
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.oe.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.oe.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  // Cleanup and return result
  eP->release();
  delete eP;

  return answer;
}
