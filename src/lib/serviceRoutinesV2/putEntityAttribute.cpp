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
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutinesV2/putEntityAttribute.h"
#include "rest/OrionError.h"
#include "parse/forbiddenChars.h"



/* ****************************************************************************
*
* putEntityAttribute -
*
* PUT /v2/entities/<id>/attrs/<attrName>
*
* Payload In:  None
* Payload Out: Entity Attribute
*
*
* 01. Fill in UpdateContextRequest
* 02. Call standard op postQueryContext
* 03. Check output from mongoBackend - any errors?
* 04. Prepare HTTP headers
* 05. Cleanup and return result
*/
std::string putEntityAttribute
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  answer;
  std::string  entityId       = compV[2];
  std::string  attributeName  = compV[4];
  std::string  type           = ciP->uriParam["type"];

  if (forbiddenIdChars(ciP->apiVersion, entityId.c_str() , NULL))
  {
    OrionError oe(SccBadRequest, INVAL_CHAR_URI);
    return oe.render(ciP, "");
  }

  if (forbiddenIdChars(ciP->apiVersion, attributeName.c_str() , NULL))
  {
    OrionError oe(SccBadRequest, INVAL_CHAR_URI);
    return oe.render(ciP, "");
  }

  // 01. Fill in UpdateContextRequest from URL and payload
  parseDataP->attr.attribute.name = attributeName;

  parseDataP->upcr.res.fill(entityId, &parseDataP->attr.attribute, "UPDATE", type);

  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);


  // 03. Check output from mongoBackend - any errors?
  if (parseDataP->upcrs.res.contextElementResponseVector.size() == 1)
  {
    if (parseDataP->upcrs.res.contextElementResponseVector[0]->statusCode.code != SccOk)
    {
      ciP->httpStatusCode = parseDataP->upcrs.res.contextElementResponseVector[0]->statusCode.code;
    }
  }


  // 04. Prepare HTTP headers
  if ((ciP->httpStatusCode == SccOk) || (ciP->httpStatusCode == SccNone))
  {
    ciP->httpStatusCode = SccNoContent;
  }


  // 05. Cleanup and return result
  parseDataP->upcr.res.release();
  parseDataP->upcrs.res.release();

  if (ciP->httpStatusCode == SccInvalidModification)
  {
    std::string  details = "Request payload is missing some piece of information. Please, check Orion documentation."; 
    OrionError   orionError(SccInvalidModification, details);     

    TIMED_RENDER(answer = orionError.render(ciP, ""));
  }

  return answer;
}
