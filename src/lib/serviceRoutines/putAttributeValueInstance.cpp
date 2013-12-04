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
  ContextAttribute*               attributeP    = new ContextAttribute(attributeName, "", "false");
  bool                            idFound       = false;
  ContextElement                  ce;
  HttpStatusCode                  s;

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

        out = restErrorReplyGet(ciP, ciP->outFormat, "", "StatusCode", SccBadRequest, "unmatching metadata ID value URI/payload", valueId + " vs " + mP->value);
        return out;
      }
      else
        idFound = true;
    }
  }

  // Copy the metadata vector of the input payload
  attributeP->metadataVector.fill(upcarP->metadataVector);

  // If no "ID" metadata was in the payload, add it
  if (idFound == false)
  {
    Metadata* mP = new Metadata("ID", "", valueId);
    attributeP->metadataVector.push_back(mP);
  }

  // Filling the rest of the structure for mongoUpdateContext
  ce.entityId.fill(entityId, "", "false");
  ce.attributeDomainName.set("");
  ce.contextAttributeVector.push_back(attributeP);
  request.contextElementVector.push_back(&ce);
  request.updateActionType.set("UPDATE");

  s = mongoUpdateContext(&request, &response);
  
  LM_M(("Responses: %d", response.contextElementResponseVector.size()));
  return "ok";
}
