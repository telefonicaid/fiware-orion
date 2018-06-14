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
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/deleteAttributeValueInstanceWithTypeAndId.h"



/* ****************************************************************************
*
* deleteAttributeValueInstanceWithTypeAndId - 
*
* DELETE /v1/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}/{metaID}
* DELETE /ngsi10/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}/{metaID}
*
* Mapped Standard Operation: UpdateContextRequest/DELETE
*
* URI params:
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 01. URI parameters
* 02. Check validity of URI params
* 03. Fill in UpdateContextRequest
* 04. Call postUpdateContext standard service routine
* 05. Translate UpdateContextResponse to StatusCode
* 06. Cleanup and return result
*/
std::string deleteAttributeValueInstanceWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  StatusCode              response;
  std::string             answer;
  std::string             entityTypeFromUriParam;
  std::string             entityTypeFromPath = compV[3];
  std::string             entityId           = compV[5];
  std::string             attributeName      = compV[7];
  std::string             metaId             = compV[8];


  // 01. URI parameters
  entityTypeFromUriParam    = ciP->uriParam[URI_PARAM_ENTITY_TYPE];


  // 02. Check validity of URI params
  if ((entityTypeFromUriParam != "") && (entityTypeFromUriParam != entityTypeFromPath))
  {
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");

    response.fill(SccBadRequest, "non-matching entity::types in URL");

    TIMED_RENDER(answer = response.render(false, false));

    return answer;
  }


  // 03. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(entityId, entityTypeFromPath, "false", attributeName, metaId, ActionTypeDelete);


  // 04. Call postUpdateContext standard service routine
  postUpdateContext(ciP, components, compV, parseDataP);


  // 05. Translate UpdateContextResponse to StatusCode
  response.fill(parseDataP->upcrs.res);


  // 06. Cleanup and return result
  TIMED_RENDER(answer = response.render(false, false));
  response.release();
  parseDataP->upcr.res.release();

  return answer;
}
