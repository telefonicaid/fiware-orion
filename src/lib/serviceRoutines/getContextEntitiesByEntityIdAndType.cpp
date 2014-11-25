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

#include "convenienceMap/mapGetContextEntitiesByEntityId.h"
#include "ngsi/ParseData.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/getContextEntitiesByEntityId.h"



/* ****************************************************************************
*
* getContextEntitiesByEntityIdAndType - 
*
* Invoked by:
*  GET /v1/registry/contextEntities/type/{ETYPE}/id/{EID}
*/
std::string getContextEntitiesByEntityIdAndType
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                          entityType = compV[4];
  std::string                          entityId   = compV[6];
  std::string                          answer;
  DiscoverContextAvailabilityResponse  response;

  LM_T(LmtConvenience, ("CONVENIENCE: got a 'GET' request for entityId '%s', entityType '%s'", entityId.c_str(), entityType.c_str()));

  ciP->httpStatusCode = mapGetContextEntitiesByEntityId(entityId, entityType, &response, ciP);
  answer = response.render(DiscoverContextAvailability, ciP->outFormat, "");
  response.release();

  return answer;
}
