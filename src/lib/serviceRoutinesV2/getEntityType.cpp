/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "rest/OrionError.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "serviceRoutinesV2/getEntityType.h"
#include "mongoBackend/mongoQueryTypes.h"



/* ****************************************************************************
*
* getEntityType -
*
* GET /v2/types/<entityType>
*
* Payload In:  None
* Payload Out: EntityTypeResponse
*
* URI parameters:
*   - options=noAttrDetail
*
*/
std::string getEntityType
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  EntityTypeResponse  response;
  std::string         entityTypeName = compV[2];
  std::string         answer;
  bool                noAttrDetail   = ciP->uriParamOptions[OPT_NO_ATTR_DETAIL];

  if (entityTypeName == "")
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_TYPE, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  TIMED_MONGO(mongoAttributesForEntityType(entityTypeName,
                                           &response,
                                           ciP->tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           noAttrDetail,
                                           ciP->apiVersion));

  if (response.entityType.count == 0)
  {
    OrionError oe(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY_TYPE, ERROR_NOT_FOUND);
    TIMED_RENDER(answer = oe.toJson());
    ciP->httpStatusCode = oe.code;
  }
  else
  {
    TIMED_RENDER(answer = response.toJson());
  }

  response.release();

  return answer;
}
