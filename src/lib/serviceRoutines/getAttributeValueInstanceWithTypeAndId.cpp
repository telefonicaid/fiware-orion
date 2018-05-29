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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/uriParamNames.h"
#include "convenience/ContextAttributeResponse.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/getAttributeValueInstanceWithTypeAndId.h"



/* ****************************************************************************
*
* getAttributeValueInstanceWithTypeAndId -
*
* GET /v1/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}/{metaID}
*
* Payload In:  None
* Payload Out: ContextAttributeResponse
*
* Mapped Standard Operation: POST QueryContextRequest
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=XXX          (must coincide with entity::type in URI PATH)
*   - !exist=entity::type       (cannot be set)
*   - exist=entity::type        (ignored)
*   - offset=XXX
*   - limit=XXX
*   - details=on
*
* 01. Get values from URL (path and parameters)
* 02. Check consistency for entity::type
* 03. Fill in QueryContextRequest
* 04. Call standard operation postQueryContext
* 05. Fill in ContextElementResponse
* 06. Cleanup and return result
*/
std::string getAttributeValueInstanceWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  ContextAttributeResponse response;
  std::string              answer;
  std::string              entityTypeFromPath   = compV[3];
  std::string              entityId             = compV[5];
  std::string              attributeName        = compV[7];
  std::string              metaID               = compV[8];
  std::string              entityTypeFromParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  EntityTypeInfo           typeInfo             = EntityTypeEmptyOrNotEmpty;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);


  // 01. Get values from URL (entityId::type, exist, !exist)
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 02. Check consistency for entity::type
  if (typeInfo == EntityTypeEmpty)
  {
    response.statusCode.fill(SccBadRequest, "entity::type cannot be empty for this request");
    alarmMgr.badInput(clientIp, "entity::type cannot be empty for this request");
  }
  else if ((entityTypeFromParam != "") && (entityTypeFromParam != entityTypeFromPath))
  {
    response.statusCode.fill(SccBadRequest, "non-matching entity::types in URL");
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");
  }
  else
  {
    // 03. Fill in QueryContextRequest
    parseDataP->qcr.res.fill(entityId, entityTypeFromParam, "false", typeInfo, attributeName);


    // 04. Call standard operation postQueryContext
    postQueryContext(ciP, components, compV, parseDataP);


    // 05. Fill in response
    response.fill(&parseDataP->qcrs.res, entityId, entityTypeFromPath, attributeName, metaID);
  }

  TIMED_RENDER(answer = response.render(ciP->apiVersion, asJsonObject, AttributeValueInstance));

  parseDataP->qcr.res.release();
  parseDataP->qcrs.res.release();
  response.release();

  return answer;
}
