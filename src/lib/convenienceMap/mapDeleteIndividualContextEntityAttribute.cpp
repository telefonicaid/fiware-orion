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

#include "logMsg/logMsg.h"

#include "convenienceMap/mapDeleteIndividualContextEntityAttribute.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* mapDeleteIndividualContextEntityAttribute - 
*/
HttpStatusCode mapDeleteIndividualContextEntityAttribute
(
  const std::string&  entityId,
  const std::string&  attributeName,
  StatusCode*         response,
  ConnectionInfo*     ciP
)
{
  HttpStatusCode         ms;
  UpdateContextRequest   ucRequest;
  UpdateContextResponse  ucResponse;
  ContextElement         ce;
  ContextAttribute       contextAttribute(attributeName, "", "");

  ce.entityId.fill(entityId, "", "false");
  ce.contextAttributeVector.push_back(&contextAttribute);
  ucRequest.updateActionType.set("Delete");

  ucRequest.contextElementVector.push_back(&ce);

  ms = mongoUpdateContext(&ucRequest, &ucResponse, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  if (ms == SccOk)
  {
    *response = ucResponse.contextElementResponseVector.get(0)->statusCode;
  }

  return ms;
}
