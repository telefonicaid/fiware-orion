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

#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/deleteAttributeValueInstance.h"



/* ****************************************************************************
*
* deleteAttributeValueInstance - 
*
* DELETE /ngsi10/contextEntities/{entityID}/attributes/{attributeName}/{valueID}
*/
std::string deleteAttributeValueInstance(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  UpdateContextRequest            request;
  UpdateContextResponse           response;
  std::string                     entityId      = compV[2];
  std::string                     attributeName = compV[4];
  std::string                     valueId       = compV[5];
  ContextAttribute*               attributeP    = new ContextAttribute(attributeName, "", "false");
  ContextElement                  ce;
  HttpStatusCode                  s;
  Metadata*                       mP            = new Metadata("ID", "", valueId);

  attributeP->metadataVector.push_back(mP);
  
  ce.entityId.fill(entityId, "", "false");
  ce.attributeDomainName.set("");
  ce.contextAttributeVector.push_back(attributeP);

  request.contextElementVector.push_back(&ce);
  request.updateActionType.set("DELETE");

  response.errorCode.code = SccOk;
  s = mongoUpdateContext(&request, &response);
  
  StatusCode statusCode;
  if (response.contextElementResponseVector.size() == 0)
    statusCode.fill(SccContextElementNotFound, "The ContextElement requested is not found", entityId + "-" + attributeName);
  else if (response.contextElementResponseVector.size() == 1)
  {
     ContextElementResponse* cerP = response.contextElementResponseVector.get(0);

    if (response.errorCode.code != SccOk)
      statusCode.fill(&response.errorCode);
    else if (cerP->statusCode.code != SccOk)
       statusCode.fill(&cerP->statusCode);
    else
       statusCode.fill(SccOk, "OK", "");
  }
  else
     statusCode.fill(SccReceiverInternalError, "Internal Error", "More than one response from deleteAttributeValueInstance::mongoUpdateContext");

  return statusCode.render(ciP->outFormat, "", false);
}
