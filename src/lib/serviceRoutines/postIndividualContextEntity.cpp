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

#include "convenience/AppendContextElementRequest.h"
#include "convenience/AppendContextElementResponse.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/postIndividualContextEntity.h"



/* ****************************************************************************
*
* postIndividualContextEntity -
*
* Corresponding Standard Operation: UpdateContext/APPEND
*
* NOTE
*   This function is used for two different URLs:
*     o /v1/contextEntities
*     o /v1/contextEntities/{entityId::id}
*
* In the longer URL (with entityId::id), the payload (AppendContextElementRequest) cannot contain any
* entityId data (id, type, isPattern).
* In the first case, the entityId data of the payload is mandatory.
* entityId::type can be empty, as always, but entityId::id MUST be filled in.
*
* POST /v1/contextEntities
* POST /ngsi10/contextEntities
* POST /v1/contextEntities/{entityId::id}
* POST /ngsi10/contextEntities/{entityId::id}
*
* Payload In:  AppendContextElementRequest
* Payload Out: AppendContextElementResponse
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*
* 00. Take care of URI params
* 01. Check that total input in consistent and correct
* 02. Fill in UpdateContextRequest from AppendContextElementRequest + URL-data + URI params
* 03. Call postUpdateContext standard service routine
* 04. Translate UpdateContextResponse to AppendContextElementResponse
* 05. Cleanup and return result
*/
std::string postIndividualContextEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  AppendContextElementRequest*  reqP                  = &parseDataP->acer.res;
  AppendContextElementResponse  response;
  std::string                   entityIdFromPayload   = reqP->entity.id;
  std::string                   entityIdFromURL       = ((compV.size() == 3) || (compV.size() == 4))? compV[2] : "";
  std::string                   entityId;
  std::string                   entityTypeFromPayload = reqP->entity.type;
  std::string                   entityTypeFromURL     = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string                   entityType;
  std::string                   answer;
  std::string                   out;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  //
  // 01. Check that total input in consistent and correct
  //

  // 01.01. entityId::id
  if ((entityIdFromPayload != "") && (entityIdFromURL != "") && (entityIdFromPayload != entityIdFromURL))
  {
    std::string error = "entityId::id differs in URL and payload";

    alarmMgr.badInput(clientIp, error);
    response.errorCode.fill(SccBadRequest, error);

    TIMED_RENDER(out = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));
    return out;
  }
  entityId = (entityIdFromPayload != "")? entityIdFromPayload : entityIdFromURL;

  // 01.02. entityId::type
  if ((entityTypeFromPayload != "") && (entityTypeFromURL != "") && (entityTypeFromPayload != entityTypeFromURL))
  {
    std::string error = "entityId::type differs in URL and payload";

    alarmMgr.badInput(clientIp, error);
    response.errorCode.fill(SccBadRequest, error);

    TIMED_RENDER(out = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));
    return out;
  }
  entityType = (entityTypeFromPayload != "")? entityTypeFromPayload :entityTypeFromURL;


  // 01.03. entityId::isPattern
  if (reqP->entity.isPattern == "true")
  {
    std::string error = "entityId::isPattern set to true in contextUpdate convenience operation";

    alarmMgr.badInput(clientIp, error);
    response.errorCode.fill(SccBadRequest, error);

    TIMED_RENDER(out = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));
    return out;
  }

  // 01.04. Entity::id must be present, somewhere ...
  if (entityId == "")
  {
    std::string error = "invalid request: mandatory entityId::id missing";

    alarmMgr.badInput(clientIp, error);
    response.errorCode.fill(SccBadRequest, error);

    TIMED_RENDER(out = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));
    return out;
  }

  // Now, forward Entity to response
  response.entity.fill(entityId, entityType, "false");


  //
  // 02. Fill in UpdateContextRequest from AppendContextElementRequest + URL-data + URI params
  //
  parseDataP->upcr.res.fill(&parseDataP->acer.res, entityId, entityType);


  // 03. Call postUpdateContext standard service routine
  postUpdateContext(ciP, components, compV, parseDataP);


  // 04. Translate UpdateContextResponse to AppendContextElementResponse
  response.fill(&parseDataP->upcrs.res);

  // 05. Cleanup and return result
  TIMED_RENDER(answer = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));

  response.release();
  parseDataP->upcr.res.release();

  return answer;
}
