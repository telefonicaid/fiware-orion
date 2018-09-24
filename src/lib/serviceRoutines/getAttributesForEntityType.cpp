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

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "orionTypes/EntityTypeResponse.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/getAttributesForEntityType.h"

#include "mongoBackend/mongoQueryTypes.h"



/* ****************************************************************************
*
* getAttributesForEntityType -
*
* GET /v1/contextTypes/{entity::type}
*
* Payload In:  None
* Payload Out: EntityTypeResponse
*
* URI parameters:
*   - attributesFormat=object
*
*/
std::string getAttributesForEntityType
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  EntityTypeResponse  response;
  std::string         entityTypeName = compV[2];

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  response.statusCode.fill(SccOk);

  //
  // NOTE
  //   The second to last parameter for mongoAttributesForEntityType 'bool noAttrDetail' is always
  //   set to true (meaning to skip the attribute detail) for NGSIv1 requests.
  //   The parameter is only used for NGSIv2.
  //
  TIMED_MONGO(mongoAttributesForEntityType(entityTypeName, &response, ciP->tenant, ciP->servicePathV, ciP->uriParam, true, ciP->apiVersion));

  std::string rendered;
  TIMED_RENDER(rendered = response.render(asJsonObject,
                                          ciP->outMimeType == JSON,
                                          ciP->uriParam[URI_PARAM_COLLAPSE] == "true"));

  response.release();

  return rendered;
}
