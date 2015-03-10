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
#include "rest/ConnectionInfo.h"
#include "convenience/UpdateContextElementResponse.h"
#include "convenienceMap/mapPutIndividualContextEntity.h"
#include "serviceRoutines/putAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/postUpdateContext.h"



/* ****************************************************************************
*
* putAllEntitiesWithTypeAndId - 
*
* PUT /v1/contextEntities/type/{entity::type}/id/{entity::id}
*
* Payload In:  UpdateContextElementRequest
* Payload Out: UpdateContextElementResponse
*
* URI parameters:
*   - attributesFormat=object
*
* 01. Fill in UpdateContextRequest
* 02. Call Standard Operation
* 03. Fill in response from UpdateContextResponse
* 04. Cleanup and return result
*/
extern std::string putAllEntitiesWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                   entityType            = compV[3];
  std::string                   entityId              = compV[5];
  std::string                   answer;
  UpdateContextElementResponse  response;

  // FIXME P1: AttributeDomainName skipped
  // FIXME P1: domainMetadataVector skipped


  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(&parseDataP->ucer.res, entityId, entityType);


  // 02. Call Standard Operation
  answer = postUpdateContext(ciP, components, compV, parseDataP);


  // 03. Fill in response from UpdateContextResponse
  response.fill(&parseDataP->upcrs.res);


  // 04. Cleanup and return result
  answer = response.render(ciP, IndividualContextEntity, "");
  response.release();

  return answer;
}
