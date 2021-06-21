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
#include "ngsi9/RegisterContextRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postContextEntitiesByEntityIdAndType.h"



/* ****************************************************************************
*
* postContextEntitiesByEntityIdAndType - 
*
* POST /v1/registry/contextEntities/type/{entity::type}/id/{entity::id}
*
* Payload In:  RegisterProviderRequest
* Payload Out: RegisterContextResponse
*
* URI parameters:
*   - entity::type=TYPE (must coincide with type in URL-path)
*   - !exist=entity::type  (if set - error -- entity::type cannot be empty)
*   - exist=entity::type   (not supported - ok if present, ok if not present ...)
*
* 01. Get values from URL (entityId::type, exist, !exist)
* 02. Check validity of URI params
* 03. Fill in RegisterContextRequest from RegisterProviderRequest
* 04. Call standard op postRegisterContext
* 05. Cleanup and return answer
*/
std::string postContextEntitiesByEntityIdAndType
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string              entityType            = compV[4];
  std::string              entityId              = compV[6];
  EntityTypeInfo           typeInfo              = EntityTypeEmptyOrNotEmpty;
  std::string              typeNameFromUriParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string              answer;
  RegisterContextResponse  response;

  // 01. Get values from URL (entityId::type, exist, !exist)
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 02. Check validity of URI params
  if (typeInfo == EntityTypeEmpty)
  {
    alarmMgr.badInput(clientIp, "entity::type cannot be empty for this request");

    response.errorCode.fill(SccBadRequest, "entity::type cannot be empty for this request");
    response.registrationId.set("000000000000000000000000");

    TIMED_RENDER(answer = response.toJsonV1());

    return answer;
  }
  else if ((typeNameFromUriParam != entityType) && (!typeNameFromUriParam.empty()))
  {
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");

    response.errorCode.fill(SccBadRequest, "non-matching entity::types in URL");
    response.registrationId.set("000000000000000000000000");

    TIMED_RENDER(answer = response.toJsonV1());

    return answer;
  }


  // 03. Fill in RegisterContextRequest from RegisterProviderRequest
  parseDataP->rcr.res.fill(parseDataP->rpr.res, entityId, entityType, "");


  // 04. Call standard op postRegisterContext
  answer = postRegisterContext(ciP, components, compV, parseDataP);


  // 05. Cleanup and return answer
  parseDataP->rcr.res.release();

  return answer;
}
