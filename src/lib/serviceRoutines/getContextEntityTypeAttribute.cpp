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

#include "ngsi/ParseData.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/getContextEntityTypeAttribute.h"



/* ****************************************************************************
*
* getContextEntityTypeAttribute - 
*/
std::string getContextEntityTypeAttribute(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  std::string                          entityType     = compV[2];
  std::string                          attributeName  = compV[4];
  std::string                          answer;
  DiscoverContextAvailabilityRequest*  requestP = &parseDataP->dcar.res;
  EntityId                             entityId(".*", entityType, "true");
  
  LM_T(LmtConvenience, ("CONVENIENCE: got a  'GET' request for entity type '%s', attribute: '%s'", entityType.c_str(), attributeName.c_str()));

  requestP->entityIdVector.push_back(&entityId);
  requestP->attributeList.push_back(attributeName);

  return postDiscoverContextAvailability(ciP, components, compV, parseDataP);
}
