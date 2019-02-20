/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/postIndividualContextEntityAttribute.h"



/* ****************************************************************************
*
* postIndividualContextEntityAttributeWithTypeAndId - 
*
* POST /v1/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}
*
* Payload In:  UpdateContextAttributeRequest
* Payload Out: StatusCode
* 
* URI parameters:
*   - entity::type=XXX        (must coincide with entity::type in URL)
*   - !exist=entity::type     (if set - error -- entity::type cannot be empty)
*   - exist=entity::type      (not supported - ok if present, ok if not present ...)
*   x attributesFormat=object (cannot be supported as the response is a StatusCode)
*
* 01. Get values from URL (entityId::type, esist, !exist)
* 02. Check validity of URI params
* 03. Fill in UpdateContextRequest
* 04. Call standard operation postUpdateContext
* 05. Translate UpdateContextResponse to StatusCode
* 06. Cleanup and return result
*/
std::string postIndividualContextEntityAttributeWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string     entityType              = compV[3];
  std::string     entityId                = compV[5];
  std::string     attributeName           = compV[7];
  std::string     entityTypeFromUriParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  EntityTypeInfo  typeInfo                = EntityTypeEmptyOrNotEmpty;
  std::string     answer;
  StatusCode      response;

  // 01. Get values from URL (entityId::type, esist, !exist)
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 02. Check validity of URI params ...
  //     And if OK;
  // 03. Fill in UpdateContextRequest
  // 04. Call standard operation postUpdateContext
  // 05. Translate UpdateContextResponse to StatusCode
  //
  if (typeInfo == EntityTypeEmpty)
  {
    response.fill(SccBadRequest, "entity::type cannot be empty for this request");
    alarmMgr.badInput(clientIp, "entity::type cannot be empty for this request");
  }
  else if ((entityTypeFromUriParam != entityType) && (entityTypeFromUriParam != ""))
  {
    response.fill(SccBadRequest, "non-matching entity::types in URL");
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");
  }
  else
  {
    // 03. Fill in UpdateContextRequest
    parseDataP->upcr.res.fill(&parseDataP->upcar.res, entityId, entityType, attributeName, ActionTypeAppend);

    // 04. Call standard operation postUpdateContext
    postUpdateContext(ciP, components, compV, parseDataP);

    // 05. Translate UpdateContextResponse to StatusCode
    response.fill(parseDataP->upcrs.res);
    parseDataP->upcr.res.release();
  }


  // 06. Cleanup and return result
  TIMED_RENDER(answer = response.toJsonV1(false, false));

  parseDataP->upcar.res.release();
  parseDataP->upcrs.res.release();

  return answer;
}
