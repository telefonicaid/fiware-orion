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
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/putEntity.h"
#include "serviceRoutinesV2/serviceRoutinesCommon.h"
#include "serviceRoutinesV2/postUpdateContext.h"
#include "rest/OrionError.h"
#include "parse/forbiddenChars.h"


/* ****************************************************************************
*
* putEntity - 
*
* PUT /v2/entities/<id>/attrs
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   - 
*
* 01. Fill in UpdateContextRequest
* 02. Call standard op putUpdateContext
* 03. Check output from mongoBackend - any errors?
* 04. Prepare HTTP headers
* 05. Cleanup and return result
*/
std::string putEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*     eP     = &parseDataP->ent.res;

  eP->entityId.id   = compV[2];
  eP->entityId.type = ciP->uriParam["type"];

  if (forbiddenIdChars(compV[2].c_str() , NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, ActionTypeReplace);


  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);

// Adjust error code if needed
  adaptErrorCodeForSingleEntityOperation(&(parseDataP->upcrs.res.error), false);

  // 03. Check error
  std::string answer = "";
  if ((parseDataP->upcrs.res.error.code != SccNone ) && (parseDataP->upcrs.res.error.code != SccOk))
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.error.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.error.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  // 04. Cleanup and return result
  eP->release();

  return answer;
}
