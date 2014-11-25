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

#include "convenienceMap/mapDeleteIndividualContextEntityAttributes.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* mapDeleteIndividualContextEntityAttributes - 
*/
HttpStatusCode mapDeleteIndividualContextEntityAttributes
(
  const std::string&  entityId,
  StatusCode*         response,
  ConnectionInfo*     ciP
)
{
  HttpStatusCode         ms;

  UpdateContextRequest   ucRequest;
  UpdateContextResponse  ucResponse;
  ContextElement*        ceP = new ContextElement();

  ceP->entityId.fill(entityId, "", "false");

  ucRequest.contextElementVector.push_back(ceP);
  ucRequest.updateActionType.set("Delete");

  ms = mongoUpdateContext(&ucRequest, &ucResponse, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  *response = ucResponse.contextElementResponseVector.get(0)->statusCode;

  ucRequest.release();
  ucResponse.release();

  return ms;
}
