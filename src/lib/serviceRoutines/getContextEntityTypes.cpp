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

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/getContextEntityTypes.h"



/* ****************************************************************************
*
* getContextEntityTypes -
*
* GET /v1/registry/contextEntityTypes/{entityId::type}
* GET /v1/registry/contextEntityTypes/{entityId::type}/attributes
* GET /ngsi9/contextEntityTypes/{entityId::type}
* GET /ngsi9/contextEntityTypes/{entityId::type}/attributes
*
* Payload In:  None
* Payload Out: DiscoverContextAvailabilityResponse
*
* 1. Fill in DiscoverContextAvailabilityRequest
* 2. Call postDiscoverContextAvailability
*/
std::string getContextEntityTypes
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  // Other similar functions use a condition based on compV.size(), but in this
  // case is ambiguous so we use compV[0]
  std::string  typeName     = (compV[0] == "v1")? compV[3] : compV[2];
  std::string  answer;

  //
  // 1. Fill in parseDataP->dcar.res to pass to postDiscoverContextAvailability
  //
  EntityId                             eId(".*", typeName, "true");
  std::vector<std::string>             attributeV;
  Restriction                          restriction;

  parseDataP->dcar.res.fill(eId, attributeV, restriction);


  //
  // 2. Call the standard operation
  //
  answer = postDiscoverContextAvailability(ciP, components, compV, parseDataP);

  return answer;
}
