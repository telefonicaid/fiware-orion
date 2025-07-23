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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/OrionError.h"
#include "serviceRoutinesV2/postEntity.h"
#include "serviceRoutinesV2/postUpdateContext.h"
#include "parse/forbiddenChars.h"


/* ****************************************************************************
*
* postEntity -
*
* POST /v2/entities/{entityId}/attrs
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   op:    operation
*/
std::string postEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*        eP  = &parseDataP->ent.res;
  ActionType     op;

  eP->entityId.id   = compV[2];
  eP->entityId.type = ciP->uriParam["type"];

  if (forbiddenIdChars(compV[2].c_str() , NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  if (ciP->uriParamOptions[OPT_APPEND] == true)  // pure-append
  {
    op     = ActionTypeAppendStrict;
  }
  else
  {
    op     = ActionTypeAppend;  // append or update
  }

  // Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, op);

  // Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP, NGSIV2_FLAVOUR_ONAPPEND);

  // Any error in the response?
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

  // Cleanup and return result
  eP->release();

  return answer;
}
