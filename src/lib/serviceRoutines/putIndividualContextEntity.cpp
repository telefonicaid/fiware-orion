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
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "convenience/UpdateContextElementResponse.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/putIndividualContextEntity.h"



/* ****************************************************************************
*
* putIndividualContextEntity -
*
* Corresponding Standard Operation: UpdateContext/UPDATE
*
* PUT /v1/contextEntities/{entityId::id}
* PUT /ngsi10/contextEntities/{entityId::id}
*
* Payload In:  UpdateContextElementRequest
* Payload Out: UpdateContextElementResponse
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 01. Take care of URI params
* 02. Fill in UpdateContextRequest from UpdateContextElementRequest
* 03. Call postUpdateContext standard service routine
* 04. Translate UpdateContextResponse to UpdateContextElementResponse
* 05. Cleanup and return result
*/
std::string putIndividualContextEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                   answer;
  std::string                   entityId = compV[2];
  UpdateContextElementResponse  response;
  std::string                   entityType;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  // 01. Take care of URI params
  entityType = ciP->uriParam[URI_PARAM_ENTITY_TYPE];


  // 02. Fill in UpdateContextRequest from UpdateContextElementRequest and entityId
  parseDataP->upcr.res.fill(&parseDataP->ucer.res, entityId, entityType);

  // And, set the UpdateActionType to UPDATE
  parseDataP->upcr.res.updateActionType.set("UPDATE");


  // 03. Call postUpdateContext standard service routine
  postUpdateContext(ciP, components, compV, parseDataP);


  // 04. Translate UpdateContextResponse to UpdateContextElementResponse
  response.fill(&parseDataP->upcrs.res);


  // 05. Cleanup and return result
  TIMED_RENDER(answer = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));


  response.release();
  parseDataP->upcr.res.release();

  return answer;
}
