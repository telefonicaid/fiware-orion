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

#include "convenience/ContextAttributeResponse.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/getIndividualContextEntityAttribute.h"



/* ****************************************************************************
*
* getIndividualContextEntityAttribute -
*
* GET /v1/contextEntities/{entityId::id}/attributes/{attributeName}
* GET /ngsi10/contextEntities/{entityId::id}/attributes/{attributeName}
*
* Payload In:  None
* Payload Out: ContextAttributeResponse
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
* 3. Fill in ContextAttributeResponse from QueryContextResponse
* 4. If 404 Not Found - enter request entity data into response StatusCode::details
* 5. Render the ContextAttributeResponse
* 6. Cleanup and return result
*/
std::string getIndividualContextEntityAttribute
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string               answer;
  std::string               entityId       = compV[2];
  std::string               entityType     = "";
  EntityTypeInfo            typeInfo       = EntityTypeEmptyOrNotEmpty;
  std::string               attributeName  = compV[4];
  ContextAttributeResponse  response;

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
  parseDataP->qcr.res.fill(entityId, entityType, "false", typeInfo, attributeName);


  // 2. Call standard operation
  answer = postQueryContext(ciP, components, compV, parseDataP);


  // 3. Fill in ContextAttributeResponse from QueryContextResponse
  if (parseDataP->qcrs.res.contextElementResponseVector.size() != 0)
  {
    response.contextAttributeVector.fill(parseDataP->qcrs.res.contextElementResponseVector[0]->entity.attributeVector);
    response.statusCode.fill(parseDataP->qcrs.res.contextElementResponseVector[0]->statusCode);
  }
  else
  {
    response.statusCode.fill(parseDataP->qcrs.res.errorCode);
  }


  // 4. If 404 Not Found - enter request entity data into response StatusCode::details
  if (response.statusCode.code == SccContextElementNotFound)
  {
    response.statusCode.details = "Entity id: /" + entityId + "/";
  }


  // 5. Render the ContextAttributeResponse
  TIMED_RENDER(answer = response.toJsonV1(asJsonObject, IndividualContextEntityAttribute));


  // 6. Cleanup and return result
  response.release();
  parseDataP->qcr.res.release();

  return answer;
}
