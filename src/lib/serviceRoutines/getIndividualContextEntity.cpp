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
#include "ngsi/ContextElementResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/getIndividualContextEntity.h"
#include "serviceRoutines/postQueryContext.h"



/* ****************************************************************************
*
* getIndividualContextEntity - 
*
* GET /v1/contextEntities/{entityId::id}
* GET /ngsi10/contextEntities/{entityId::id}
*
* Payload In:  None
* Payload Out: ContextElementResponse
*
* URI parameters:
*   - !exist=entity::type
*   - exist=entity::type
*   - entity::type=TYPE
*   - attributesFormat=object
*
* 0. Take care of URI params
* 1. Fill in QueryContextRequest (includes adding URI parameters as Scope in restriction)
* 2. Call standard operation
* 3. Fill in ContextElementResponse from QueryContextResponse
* 4. If 404 Not Found - enter request entity data into response context element
* 5. Render the ContextElementResponse
* 6. Cleanup and return result
*/
std::string getIndividualContextEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string             answer;
  std::string             entityId    = compV[2];
  std::string             entityType  = "";
  EntityTypeInfo          typeInfo    = EntityTypeEmptyOrNotEmpty;
  ContextElementResponse  response;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  // 0. Take care of URI params
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }
  entityType = ciP->uriParam[URI_PARAM_ENTITY_TYPE];


  // 1. Fill in QueryContextRequest (includes adding URI parameters as Scope in restriction)
  parseDataP->qcr.res.fill(entityId, entityType, "false", typeInfo, "");


  // 2. Call standard operation
  answer = postQueryContext(ciP, components, compV, parseDataP);


  // 3. Fill in ContextElementResponse from QueryContextResponse
  response.statusCode.fill(SccOk);
  response.fill(&parseDataP->qcrs.res);


  // 4. If 404 Not Found - enter request entity data into response context element
  if (response.statusCode.code == SccContextElementNotFound)
  {
    response.contextElement.entityId.fill(entityId, entityType, "false");
    response.statusCode.details = "Entity id: /" + entityId + "/";
  }


  // 5. Render the ContextElementResponse
  TIMED_RENDER(answer = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntity));


  // 6. Cleanup and return result
  response.release();
  parseDataP->qcr.res.release();

  return answer;
}
