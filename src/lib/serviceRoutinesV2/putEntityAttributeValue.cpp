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


  // 01. Fill in UpdateContextRequest with data from URI and payload
  parseDataP->av.attribute.name = attributeName;
  parseDataP->av.attribute.type = "";  // Overwrite 'none', as no type can be given in 'value' payload

  std::string err = parseDataP->av.attribute.check(ciP,ciP->requestType,"","", 0);
  if (err != "OK")
  {
    OrionError oe(SccBadRequest, err);
    return oe.render(ciP, "");
  }
  parseDataP->upcr.res.fill(entityId, &parseDataP->av.attribute, "UPDATE", type);


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

  if (ciP->httpStatusCode == SccConflict)
  {
    ErrorCode   ec("TooManyResults", MORE_MATCHING_ENT);
    std::string answer;

    TIMED_RENDER(answer = ec.toJson(true));

    return answer;
  }

  // 04. Prepare HTTP headers
  if ((ciP->httpStatusCode == SccOk) || (ciP->httpStatusCode == SccNone))
  {
    ciP->httpStatusCode = SccNoContent;
  }


  // 05. Cleanup and return result
  parseDataP->upcr.res.release();

  return "";
}
