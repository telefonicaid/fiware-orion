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

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/deleteIndividualContextEntity.h"



/* ****************************************************************************
*
* deleteIndividualContextEntity - 
*
* Corresponding Standard Operation: UpdateContext/DELETE
*
* DELETE /v1/contextEntities/{entityId::id}
*
* Payload In:  None
* Payload Out: StatusCode
*
* URI parameters:
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 00. URI params
* 01. Fill in UpdateContextRequest from URL-data + URI params
* 02. Call postUpdateContext standard service routine
* 03. Translate UpdateContextResponse to StatusCode
* 04. If not found, put entity info in details
* 05. Cleanup and return result
*/
std::string deleteIndividualContextEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  answer;
  std::string  entityId   = compV[2];
  std::string  entityType = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  StatusCode   response;

  // 01. Fill in UpdateContextRequest fromURL-data + URI params
  parseDataP->upcr.res.fill(entityId, entityType, "false", "", "", ActionTypeDelete);

  // 02. Call postUpdateContext standard service routine
  answer = postUpdateContext(ciP, components, compV, parseDataP);

  // 03. Translate UpdateContextResponse to StatusCode
  response.fill(parseDataP->upcrs.res);

  // 04. If not found, put entity info in details
  if ((response.code == SccContextElementNotFound) && (response.details == ""))
  {
    response.details = entityId;
  }

  // 05. Cleanup and return result
  TIMED_RENDER(answer = response.render(false, false));

  response.release();
  parseDataP->upcr.res.release();

  return answer;
}
