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
#include "rest/uriParamNames.h"

#include "apiTypesV2/Attribute.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutinesV2/getEntityAttribute.h"
#include "parse/forbiddenChars.h"
#include "rest/OrionError.h"

/* ****************************************************************************
*
* getEntityAttribute -
*
* GET /v2/entities/:id:/attrs/:attrName:
*
* Payload In:  None
* Payload Out: Entity Attribute
*
*
* 01. Fill in QueryContextRequest
* 02. Call standard op postQueryContext
* 03. Render Entity Attribute response
* 04. Cleanup and return result
*/
std::string getEntityAttribute
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  type   = ciP->uriParam["type"];
  std::string  answer;
  Attribute    attribute;

  if (forbiddenIdChars(ciP->apiVersion, compV[2].c_str(), NULL) ||
      forbiddenIdChars(ciP->apiVersion, compV[4].c_str(), NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  // 01. Fill in QueryContextRequest
  parseDataP->qcr.res.fill(compV[2], type, "false", EntityTypeEmptyOrNotEmpty, "");
  parseDataP->qcr.res.metadataList.fill(ciP->uriParam[URI_PARAM_METADATA]);


  // 02. Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);


  // 03. Render entity attribute response
  attribute.fill(&parseDataP->qcrs.res, compV[4]);

  TIMED_RENDER(answer = attribute.render(ciP->httpHeaders.accepted("text/plain"),
                                         ciP->httpHeaders.accepted("application/json"),
                                         ciP->httpHeaders.outformatSelect(),
                                         &(ciP->outMimeType),
                                         &(ciP->httpStatusCode),
                                         ciP->uriParamOptions[OPT_KEY_VALUES],
                                         ciP->uriParam[URI_PARAM_METADATA],
                                         EntityAttributeResponse));

  if (attribute.oe.reasonPhrase == ERROR_TOO_MANY)
  {
    ciP->httpStatusCode = SccConflict;
  }
  else if (attribute.oe.reasonPhrase == ERROR_NOT_FOUND)
  {
    ciP->httpStatusCode = SccContextElementNotFound;  // Attribute to be precise!
  }
  else
  {
    // the same of the wrapped operation
    ciP->httpStatusCode = parseDataP->qcrs.res.errorCode.code;
  }

  // 04. Cleanup and return result
  parseDataP->qcr.res.release();

  return answer;
}

