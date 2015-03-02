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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "convenience/AppendContextElementRequest.h"
#include "convenience/AppendContextElementResponse.h"
#include "convenienceMap/mapPostIndividualContextEntityAttributes.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* mapPostIndividualContextEntityAttributes -
*/
HttpStatusCode mapPostIndividualContextEntityAttributes
(
  const std::string&             entityId,
  AppendContextElementRequest*   request,
  AppendContextElementResponse*  response,
  ConnectionInfo*                ciP
)
{
  HttpStatusCode         ms;

  UpdateContextRequest   ucRequest;
  UpdateContextResponse  ucResponse;
  ContextElement         ce;

  ce.entityId.fill(entityId, "", "false");
  ce.attributeDomainName    = request->attributeDomainName;
  ce.contextAttributeVector = request->contextAttributeVector;

  ucRequest.contextElementVector.push_back(&ce);
  ucRequest.updateActionType.set("Append");

  ms = mongoUpdateContext(&ucRequest, &ucResponse, ciP->tenant, ciP->servicePathV, ciP->uriParam, ciP->httpHeaders.xauthToken);

  ContextAttributeResponse* car                      = new ContextAttributeResponse();
  ContextElementResponse*   ucContextElementResponse = ucResponse.contextElementResponseVector.get(0);

  // Copying contextAttributeVector from ucContextElementResponse
  for (unsigned caIx = 0; caIx < ucContextElementResponse->contextElement.contextAttributeVector.size(); ++caIx)
  {
    LM_T(LmtClone, ("Copying ContextAttribute %d", caIx));
    ContextAttribute* attribute = ucContextElementResponse->contextElement.contextAttributeVector[caIx];
    ContextAttribute* caP = new ContextAttribute(attribute);

    car->contextAttributeVector.push_back(caP);
  }
  car->statusCode.fill(&ucContextElementResponse->statusCode);

  response->contextAttributeResponseVector.push_back(car);
  response->errorCode.fill(&ucResponse.contextElementResponseVector.get(0)->statusCode);

  return ms;
}
