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
#include "ngsi/EntityId.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "convenience/UpdateContextElementResponse.h"
#include "serviceRoutines/putAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/postUpdateContext.h"



/* ****************************************************************************
*
* putAllEntitiesWithTypeAndId - 
*
* PUT /v1/contextEntities/type/{entity::type}/id/{entity::id}
*
* Payload In:  UpdateContextElementRequest
* Payload Out: UpdateContextElementResponse
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=TYPE (must coincide with type in URL-path)
*   - !exist=entity::type  (if set - error -- entity::type cannot be empty)
*   - exist=entity::type   (not supported - ok if present, ok if not present ...)
*
* 01. Get values from URL (entityId::type, exist, !exist)
* 02. Check validity of URI params
* 03. Fill in UpdateContextRequest
* 04. Call Standard Operation
* 05. Fill in response from UpdateContextResponse
* 06. Cleanup and return result
*/
extern std::string putAllEntitiesWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                   entityType            = compV[3];
  std::string                   entityId              = compV[5];
  EntityTypeInfo                typeInfo              = EntityTypeEmptyOrNotEmpty;
  std::string                   typeNameFromUriParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string                   answer;
  UpdateContextElementResponse  response;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  // 01. Get values from URL (entityId::type, esist, !exist)
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
    TIMED_RENDER(answer = response.toJsonV1(asJsonObject, AllEntitiesWithTypeAndId));
    return answer;
  }
  else if ((typeNameFromUriParam != entityType) && (!typeNameFromUriParam.empty()))
  {
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL", typeNameFromUriParam);
    response.errorCode.fill(SccBadRequest, "non-matching entity::types in URL");
    TIMED_RENDER(answer = response.toJsonV1( asJsonObject, AllEntitiesWithTypeAndId));
    return answer;
  }


  // 03. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(&parseDataP->ucer.res, entityId, entityType);


  // 04. Call Standard Operation
 postUpdateContext(ciP, components, compV, parseDataP);


  // 05. Fill in response from UpdateContextResponse
  response.fill(&parseDataP->upcrs.res);


  // 06. Cleanup and return result
  TIMED_RENDER(answer = response.toJsonV1(asJsonObject, IndividualContextEntity));

  parseDataP->upcr.res.release();
  response.release();

  return answer;
}
