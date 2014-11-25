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

#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "convenience/ContextAttributeResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/getAttributeValueInstanceWithTypeAndId.h"



/* ****************************************************************************
*
* getAttributeValueInstanceWithTypeAndId - 
*
* GET /v1/contextEntities/type/{type}/id/{id}/attributes/{attributeName}/{valueID}
*/
std::string getAttributeValueInstanceWithTypeAndId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  QueryContextRequest      request;
  QueryContextResponse     response;
  std::string              entityType    = compV[3];
  std::string              entityId      = compV[5];
  std::string              attributeName = compV[7];
  std::string              valueID       = compV[8];
  EntityId*                eP            = new EntityId(entityId, entityType, "false");
  StatusCode               sc;
  ContextAttributeResponse car;

  request.entityIdVector.push_back(eP);
  request.attributeList.push_back(attributeName);

  ciP->httpStatusCode = mongoQueryContext(&request, &response, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  if (response.contextElementResponseVector.size() == 0)
  {
     car.statusCode.fill(SccContextElementNotFound,
                         std::string("Entity-Attribute pair: /") + entityId + "-" + attributeName + "/");
  }
  else
  {
    ContextElementResponse* cerP = response.contextElementResponseVector.get(0);
    ContextAttributeVector cav = cerP->contextElement.contextAttributeVector;

    // FIXME P4: as long as mongoQueryContext() signature is based on NGSI standard operations and that
    // standard queryContext doesn't allow specify metadata for attributes (note that it uses xs:string,
    // not full fledge attribute types), we cannot pass the ID to mongoBackend so we need to do the for loop
    // to grep the right attribute among all the ones returned by mongoQueryContext. However, this involves
    // a suboptimal query at mongoBackend, which could be improved passing it the ID as a new parameter to
    // mongoQueryContext() (although breaking the design principle about mongo*() functions follow the NGSI
    // standard). To think about it.
    for (unsigned int i = 0; i < cav.size(); i++)
    {
      if (cav.get(i)->getId() == valueID)
      {
        car.contextAttributeVector.push_back(cav.get(i));
      }
    }

    if (cav.size() > 0 && car.contextAttributeVector.size() == 0)
    {
      car.statusCode.fill(SccContextElementNotFound,
                          std::string("Attribute-ValueID pair: /") + attributeName + "-" + valueID + "/");
    }
    else
    {
      car.statusCode.fill(&cerP->statusCode);
    }
  }

  request.release();

  return car.render(ciP, AttributeValueInstance, "");
}
