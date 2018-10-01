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
#include "rest/uriParamNames.h"
#include "rest/restReply.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/postAttributeValueInstanceWithTypeAndId.h"



/* ****************************************************************************
*
* postAttributeValueInstanceWithTypeAndId -
*
* POST /v1/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}/{metaID}
* POST /ngsi10/contextEntities/type/{entity::type}/id/{entity::id}/attributes/{attribute::name}/{metaID}
*
* Payload In:  UpdateContextAttributeRequest
* Payload Out: StatusCode
*
* Mapped Standard Operation: UpdateContextRequest/APPEND
*
* URI parameters
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 00. Get values from URI path
* 01. Get values URI parameters
* 02. Check validity of URI params VS URI path components
* 03. Check validity of path components VS payload
* 04. Fill in UpdateContextRequest
* 05. Call postUpdateContext
* 06. Fill in StatusCode from UpdateContextResponse
* 07. Render result
* 08. Cleanup and return result
*/
std::string postAttributeValueInstanceWithTypeAndId
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
  std::string     metaID                  = compV[8];
  std::string     entityTypeFromUriParam;
  StatusCode      response;
  std::string     answer;


  // 01. Get values URI parameters
  entityTypeFromUriParam  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];


  // 02. Check validity of URI params VS URI path components
  if ((entityTypeFromUriParam != "") && (entityTypeFromUriParam != entityType))
  {
    alarmMgr.badInput(clientIp, "non-matching entity::types in URL");

    response.fill(SccBadRequest, "non-matching entity::types in URL");

    TIMED_RENDER(answer = response.toJsonV1(false, false));

    parseDataP->upcar.res.release();
    return answer;
  }


  // 03. Check validity of path components VS payload
  Metadata* mP = parseDataP->upcar.res.metadataVector.lookupByName("ID");

  if ((mP != NULL) && (mP->stringValue != metaID))
  {
    std::string details = "unmatching metadata ID value URI/payload: /" + metaID + "/ vs /" + mP->stringValue + "/";

    response.fill(SccBadRequest, details);

    TIMED_RENDER(answer = response.toJsonV1(false, false));

    parseDataP->upcar.res.release();

    return answer;
  }


  // 04. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(&parseDataP->upcar.res, entityId, entityType, attributeName, metaID, ActionTypeAppend);


  // 05. Call postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);


  // 06. Fill in StatusCode from UpdateContextResponse
  response.fill(parseDataP->upcrs.res);


  // 07. Render result
  TIMED_RENDER(answer = response.toJsonV1(false, false));


  // 08. Cleanup and return result
  response.release();
  parseDataP->upcar.res.release();
  parseDataP->upcr.res.release();
  parseDataP->upcrs.res.release();

  return answer;
}
