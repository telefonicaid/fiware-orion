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
#include "logMsg/logMsg.h"

#include "convenience/UpdateContextElementRequest.h"
#include "convenience/UpdateContextElementResponse.h"
#include "convenienceMap/mapPutIndividualContextEntity.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* mapPutIndividualContextEntity - 
*/
HttpStatusCode mapPutIndividualContextEntity(std::string entityId, UpdateContextElementRequest* ucerP, UpdateContextElementResponse* response)
{
  HttpStatusCode          ms;
  UpdateContextRequest    ucRequest;
  UpdateContextResponse   ucResponse;
  ContextElement          ce;

  ce.entityId.fill(entityId, "", "false");
  ce.attributeDomainName    = ucerP->attributeDomainName;
  ce.contextAttributeVector = ucerP->contextAttributeVector;
  
  ucRequest.contextElementVector.push_back(&ce);
  ucRequest.updateActionType.set("Update");

  ms = mongoUpdateContext(&ucRequest, &ucResponse);

  if (ucResponse.contextElementResponseVector.size() == 0)
  {
     response->errorCode = ucResponse.errorCode;
     return ms;
  }

  ContextAttributeResponse* carP = new ContextAttributeResponse();
  ContextElement*           ceP  = &ucResponse.contextElementResponseVector.get(0)->contextElement;

  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
  {
    ContextAttribute* caP = new ContextAttribute(ceP->contextAttributeVector.get(ix));
    carP->contextAttributeVector.push_back(caP);
  }

  response->contextResponseVector.push_back(carP);
  response->errorCode.fill(&ucResponse.contextElementResponseVector.get(0)->statusCode);

  return ms;
}
