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

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/deleteAttributeValueInstance.h"



/* ****************************************************************************
*
* deleteAttributeValueInstance - 
*
* DELETE /v1/contextEntities/{entity::id}/attributes/{attribute::name}/{metaID}
* DELETE /ngsi10/contextEntities/{entity::id}/attributes/{attribute::name}/{metaID}
*
* Payload In:  None
* Payload Out: StatusCode
*
* Mapped Standard Operation: UpdateContextRequest/DELETE
*
* URI params:
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 01. URI parameters
* 02. Fill in UpdateContextRequest
* 03. Call postUpdateContext standard service routine
* 04. Translate UpdateContextResponse to StatusCode
* 05. Cleanup and return result
*
*/
std::string deleteAttributeValueInstance
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  StatusCode              response;
  std::string             answer;
  std::string             entityId      = compV[2];
  std::string             attributeName = compV[4];
  std::string             metaId        = compV[5];
  std::string             entityType;

  // 01. URI parameters
  entityType    = ciP->uriParam[URI_PARAM_ENTITY_TYPE];

  // 02. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(entityId, entityType, "false", attributeName, metaId, ActionTypeDelete);

  // 03. Call postUpdateContext standard service routine
  postUpdateContext(ciP, components, compV, parseDataP);


  // 04. Translate UpdateContextResponse to StatusCode
  response.fill(parseDataP->upcrs.res);


  // 05. Cleanup and return result
  TIMED_RENDER(answer = response.render(false, false));

  response.release();
  parseDataP->upcr.res.release();

  return answer;
}
