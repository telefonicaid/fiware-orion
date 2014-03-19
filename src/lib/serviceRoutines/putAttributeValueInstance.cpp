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
#include "rest/ConnectionInfo.h"
#include "rest/restReply.h"
#include "ngsi/ParseData.h"
#include "ngsi10/UpdateContextResponse.h"
#include "serviceRoutines/putAttributeValueInstance.h"



/* ****************************************************************************
*
* putAttributeValueInstance - 
*
* PUT /ngsi10/contextEntities/{entityID}/attributes/{attributeName}/{valueID}
*
* Payload: UpdateContextAttributeRequest
*/
std::string putAttributeValueInstance(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  UpdateContextRequest            request;
  UpdateContextResponse           response;
  std::string                     entityId      = compV[2];
  std::string                     attributeName = compV[4];
  std::string                     valueId       = compV[5];
  UpdateContextAttributeRequest*  upcarP        = &parseDataP->upcar.res;
  bool                            idFound       = false;

  //
  // Any metadata ID in the payload?
  //
  // If so, the value must be equal to the {valueID} of the URL
  //
  for (unsigned int ix = 0; ix < upcarP->metadataVector.size(); ++ix)
  {
    Metadata* mP = upcarP->metadataVector.get(ix);

    if (mP->name == "ID")
    {
      if (mP->value != valueId)
      {
        std::string out;

        out = restErrorReplyGet(ciP, ciP->outFormat, "", "StatusCode", SccBadRequest,
                                std::string("unmatching metadata ID value URI/payload: '") + valueId + "' vs '" + mP->value + "'");
        return out;
      }
      else
        idFound = true;
    }
  }


  ContextAttribute*               attributeP    = new ContextAttribute(attributeName, "", upcarP->contextValue);
  ContextElement*                 ceP           = new ContextElement();

  // Copy the metadata vector of the input payload
  attributeP->metadataVector.fill(upcarP->metadataVector);

  // If no "ID" metadata was in the payload, add it
  if (idFound == false)
  {
    Metadata* mP = new Metadata("ID", "", valueId);
    attributeP->metadataVector.push_back(mP);
  }

  // Filling the rest of the structure for mongoUpdateContext
  ceP->entityId.fill(entityId, "", "false");
  ceP->attributeDomainName.set("");
  ceP->contextAttributeVector.push_back(attributeP);
  request.contextElementVector.push_back(ceP);
  request.updateActionType.set("UPDATE");

  response.errorCode.code = SccNone;
  ciP->httpStatusCode = mongoUpdateContext(&request, &response);
  
  StatusCode statusCode;
  if (response.contextElementResponseVector.size() == 0)
    statusCode.fill(SccContextElementNotFound, std::string("Entity-Attribute pair: '") + entityId + "-" + attributeName + "'");
  else if (response.contextElementResponseVector.size() == 1)
  {
    ContextElementResponse* cerP = response.contextElementResponseVector.get(0);

    if (response.errorCode.code != SccNone)
      statusCode.fill(&response.errorCode);
    else if (cerP->statusCode.code != SccNone)
      statusCode.fill(&cerP->statusCode);
    else
      statusCode.fill(SccOk);
  }
  else
    statusCode.fill(SccReceiverInternalError, "More than one response from putAttributeValueInstance::mongoUpdateContext");

  request.release();
  return statusCode.render(ciP->outFormat, "", false);
}
