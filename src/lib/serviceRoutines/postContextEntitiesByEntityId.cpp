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

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postContextEntitiesByEntityId.h"



/* ****************************************************************************
*
* postContextEntitiesByEntityId - 
*
* POST /v1/registry/contextEntities/{entityId}
* POST /ngsi9/contextEntities/{entityId}
*
* Payload In:  RegisterProviderRequest
* Payload Out: RegisterContextResponse
*
* 1. Transform RegisterProviderRequest+entityId into a RegisterContextRequest
* 2. Call the Standard operation for RegisterContextRequest
*/
std::string postContextEntitiesByEntityId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  entityId = (compV.size() == 4)? compV[3] : compV[2];
  std::string  answer;

  parseDataP->rcr.res.fill(parseDataP->rpr.res, entityId, "", "");
  answer = postRegisterContext(ciP, components, compV, parseDataP);

  parseDataP->rcr.res.release();

  return answer;
}
