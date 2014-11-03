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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "convenience/AppendContextElementResponse.h"
#include "convenienceMap/mapPostIndividualContextEntity.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postIndividualContextEntity.h"



/* ****************************************************************************
*
* postIndividualContextEntity -
*
* NOTE
* This function is used for two requests:
* o POST /v1/contextEntities  and
* o POST /v1/contextEntities/{entityId::id}
*
* In the latter case, the payload (AppendContextElementRequest) cannot contain any
* entityId data (id, type, isPattern).
* In the first case, the entityId data of the payload is mandatory.
* entityId::type can be empty, as always, but entityId::id MUST be filled in.
*/
std::string postIndividualContextEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                   entityId;
  std::string                   entityType;
  std::string                   answer;
  AppendContextElementRequest*  reqP       = &parseDataP->acer.res;
  AppendContextElementResponse  response;

  response.entity = reqP->entity;

  if (compV.size() == 3)
  {
    entityId   = compV[2];
    entityType = "";

    if ((reqP->entity.id != "") || (reqP->entity.type != "") || (reqP->entity.isPattern != ""))
    {
      LM_W(("Bad Input (unknown field)"));
      response.errorCode.fill(SccBadRequest, "invalid payload: unknown fields");
      return response.render(ciP, IndividualContextEntity, "");
    }
  }
  else if (compV.size() == 2)
  {
    entityId   = reqP->entity.id;
    entityType = reqP->entity.type;

    if ((entityId == "") && (entityType == ""))
    {
      LM_W(("Bad Input (mandatory entityId::id missing)"));
      response.errorCode.fill(SccBadRequest, "invalid payload: mandatory entityId::id missing");
      return response.render(ciP, IndividualContextEntity, "");
    }
  }

  LM_T(LmtConvenience, ("CONVENIENCE: got a 'POST' request for entityId '%s'", entityId.c_str()));

  ciP->httpStatusCode = mapPostIndividualContextEntity(entityId, "", &parseDataP->acer.res, &response, ciP);

  response.entity = reqP->entity;
  answer = response.render(ciP, IndividualContextEntity, "");
  response.release();

  return answer;
}
