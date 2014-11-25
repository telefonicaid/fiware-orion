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
#include "convenience/AppendContextElementResponse.h"
#include "convenienceMap/mapPostIndividualContextEntity.h"
#include "serviceRoutines/postAllEntitiesWithTypeAndId.h"



/* ****************************************************************************
*
* postAllEntitiesWithTypeAndId - 
*/
extern std::string postAllEntitiesWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                   enType = compV[3];
  std::string                   enId   = compV[5];
  AppendContextElementRequest*  reqP   = &parseDataP->acer.res;
  std::string                   answer;
  AppendContextElementResponse  response;

  // FIXME P1: AttributeDomainName skipped
  // FIXME P1: domainMetadataVector skipped

  if ((reqP->entity.id != "") || (reqP->entity.type != "") || (reqP->entity.isPattern != ""))
  {
    LM_W(("Bad Input (unknown field)"));
    response.errorCode.fill(SccBadRequest, "invalid payload: unknown fields");
    return response.render(ciP, IndividualContextEntity, "");
  }

  LM_T(LmtConvenience, ("CONVENIENCE: got a 'POST' request for entityId '%s', type '%s'", enId.c_str(), enType.c_str()));

  ciP->httpStatusCode = mapPostIndividualContextEntity(enId, enType, &parseDataP->acer.res, &response, ciP);
  answer = response.render(ciP, IndividualContextEntity, "");
  response.release();

  return answer;
}
