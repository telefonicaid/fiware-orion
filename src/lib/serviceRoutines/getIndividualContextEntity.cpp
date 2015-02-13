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

#include "convenienceMap/mapGetIndividualContextEntity.h"
#include "ngsi/ParseData.h"
#include "ngsi/ContextElementResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/getIndividualContextEntity.h"



/* ****************************************************************************
*
* getIndividualContextEntity -
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

  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }

  entityType = ciP->uriParam[URI_PARAM_ENTITY_TYPE];

  ciP->httpStatusCode = mapGetIndividualContextEntity(entityId, entityType, typeInfo, &response, ciP);

  answer = response.render(ciP, IndividualContextEntity, "");
  response.release();

  return answer;
}
