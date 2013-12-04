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

#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "convenience/ContextAttributeResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/getAttributeValueInstance.h"



/* ****************************************************************************
*
* getAttributeValueInstance - 
*
* GET /ngsi10/contextEntities/{entityID}/attributes/{attributeName}/{valueID}
*/
std::string getAttributeValueInstance(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  QueryContextRequest      request;
  QueryContextResponse     response;
  std::string              entityId      = compV[2];
  std::string              attributeName = compV[4];
  std::string              valueId       = compV[5];
  EntityId*                eP            = new EntityId(entityId, "", "false");
  HttpStatusCode           s;
  StatusCode               sc;
  std::string              out;
  ContextAttributeResponse car;

  request.entityIdVector.push_back(eP);
  request.attributeList.push_back(attributeName);

  s = mongoQueryContext(&request, &response);
  if (s == SccOk)
    LM_M(("Got %d replies for attribute-ID %s-%s", response.contextElementResponseVector.size(), attributeName.c_str(), valueId.c_str()));

  if (response.contextElementResponseVector.size() == 0)
  {
     car.statusCode.fill(SccContextElementNotFound, "The ContextElement requested is not found", entityId + "-" + attributeName);
  }
  else
  {
     ContextElementResponse* cerP = response.contextElementResponseVector.get(0);

     car.contextAttributeVector.push_back(cerP->contextElement.contextAttributeVector.get(0));
     car.statusCode.fill(&cerP->statusCode);
  }

  return car.render(ciP->outFormat, "");
}
