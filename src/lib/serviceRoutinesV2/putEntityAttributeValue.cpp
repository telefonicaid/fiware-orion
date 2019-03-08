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
* Author: Ken Zangelin 
*/
#include <string>
#include <vector>

#include "common/statistics.h"
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutinesV2/putEntityAttributeValue.h"
#include "rest/OrionError.h"
#include "parse/forbiddenChars.h"


/* ****************************************************************************
*
* putEntityAttributeValue -
*
* PUT /v2/entities/<id>/attrs/<attrName>/value
*
* Payload In:  AttributeValue
* Payload Out: None
*
*
* 01. Fill in UpdateContextRequest with data from URI and payload
* 02. Call standard op postUpdateContext
* 03. Check output from mongoBackend - any errors?
* 04. Prepare HTTP headers
* 05. Cleanup and return result
*/
std::string putEntityAttributeValue
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  entityId       = compV[2];
  std::string  attributeName  = compV[4];
  std::string  type           = ciP->uriParam["type"];

  if (forbiddenIdChars(ciP->apiVersion, entityId.c_str(),      NULL) ||
      forbiddenIdChars(ciP->apiVersion, attributeName.c_str(), NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  // 01. Fill in UpdateContextRequest with data from URI and payload
  parseDataP->av.attribute.name = attributeName;
  parseDataP->av.attribute.type = "";  // Overwrite 'none', as no type can be given in 'value' payload
  parseDataP->av.attribute.onlyValue = true;

  std::string err = parseDataP->av.attribute.check(ciP->apiVersion, ciP->requestType);
  if (err != "OK")
  {
    OrionError oe(SccBadRequest, err, "BadRequest");
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }
  parseDataP->upcr.res.fill(entityId, &parseDataP->av.attribute, ActionTypeUpdate, type);


  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);

  // 03. Check output from mongoBackend
  std::string answer = "";
  if (parseDataP->upcrs.res.oe.code != SccNone)
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.oe.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.oe.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  // 05. Cleanup and return result
  parseDataP->upcr.res.release();

  return answer;
}
