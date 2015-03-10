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

#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/deleteAllEntitiesWithTypeAndId.h"



/* ****************************************************************************
*
* deleteAllEntitiesWithTypeAndId - 
*
* DELETE /v1/contextEntities/type/{entity::type}/id/{entity::id}
*
* Payload In:  None
* Payload Out: StatusCode
*
* URI parameters:
*   - entity::type=TYPE (must coincide with type in URL-path)
*   - !exist=entity::type  (if set - error -- entity::type cannot be empty)
*   - exist=entity::type   (not supported - ok if present, ok if not present ...)
*
* 01. Fill in UpdateContextRequest
* 02. Call Standard Operation
* 03. Fill in response from UpdateContextResponse
* 04. Cleanup and return result
*/
extern std::string deleteAllEntitiesWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  entityType  = compV[3];
  std::string  entityId    = compV[5];
  std::string  answer;
  StatusCode   response;

  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(entityId, entityType, false, "", "DELETE");

  // 02. Call Standard Operation
  postUpdateContext(ciP, components, compV, parseDataP);

  // 03. Fill in response from UpdateContextResponse
  response.fill(parseDataP->upcrs.res);

  // 04. Cleanup and return result
  answer = response.render(ciP->outFormat, "");
  response.release();

  return answer;
}
