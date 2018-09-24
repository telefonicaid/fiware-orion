/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "convenience/ContextAttributeResponse.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/getIndividualContextEntityAttributeWithTypeAndId.h"



/* ****************************************************************************
*
* getIndividualContextEntityAttributeWithTypeAndId -
*
* GET /v1/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}
*
* Payload In:  None
* Payload Out: ContextAttributeResponse
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=XXX     (must coincide with entity::type in URL)
*   - !exist=entity::type  (if set - error -- entity::type cannot be empty)
*   - exist=entity::type   (not supported - ok if present, ok if not present ...)
*
* 01. Get values from URL (entityId::type, esist, !exist)
* 02. Check validity of URI params
* 03. Fill in QueryContextRequest
* 04. Call standard operation postQueryContext
* 05. If 404 Not Found - enter request info into response context element
* 06. Translate QueryContextResponse to ContextElementResponse
* 07. Cleanup and return result
*/
std::string getIndividualContextEntityAttributeWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string               answer;
  std::string               entityType              = compV[3];
  std::string               entityId                = compV[5];
  std::string               attributeName           = compV[7];
  std::string               entityTypeFromUriParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  EntityTypeInfo            typeInfo                = EntityTypeEmptyOrNotEmpty;
  ContextAttributeResponse  response;

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


  //
  // 02. Check validity of URI params ...
  //     and if OK;
  // 03. Fill in QueryContextRequest
  // 04. Call standard operation postQueryContext
  // 05. If 404 Not Found - enter request info into response context element
  //
  if (typeInfo == EntityTypeEmpty)
  {
    parseDataP->qcrs.res.errorCode.fill(SccBadRequest, "entity::type cannot be empty for this request");
    alarmMgr.badInput(clientIp, "entity::type cannot be empty for this request");
  }
  else if ((entityTypeFromUriParam != entityType) && (entityTypeFromUriParam != ""))
  {
    parseDataP->qcrs.res.errorCode.fill(SccBadRequest, "non-matching entity::types in URL");
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");
  }
  else
  {
    // 03. Fill in QueryContextRequest
    parseDataP->qcr.res.fill(entityId, entityType, attributeName);


    // 04. Call standard operation postQueryContext
    parseDataP->qcrs.res.errorCode.fill(SccOk);
    postQueryContext(ciP, components, compV, parseDataP);

    // 05. If 404 Not Found - enter request info into response context element
    if (parseDataP->qcrs.res.errorCode.code == SccContextElementNotFound)
    {
      response.statusCode.details = "entityId::type/attribute::name pair not found";
    }
  }


  // 06. Translate QueryContextResponse to ContextAttributeResponse
  response.fill(&parseDataP->qcrs.res, entityId, entityType, attributeName, "");


  // 07. Cleanup and return result
  TIMED_RENDER(answer = response.render(asJsonObject, RtContextAttributeResponse));


  parseDataP->qcr.res.release();
  parseDataP->qcrs.res.release();
  response.release();

  return answer;
}
