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
std::string putAttributeValueInstance
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
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

        out = restErrorReplyGet(ciP,
                                ciP->outFormat,
                                "", "StatusCode",
                                SccBadRequest,
                                std::string("unmatching metadata ID value URI/payload: /") +
                                  valueId + "/ vs /" + mP->value + "/");
        return out;
      }
      else
      {
        idFound = true;
      }
    }
  }

  LM_M(("KZ: ID OK"));

  ContextAttribute*               attributeP    = new ContextAttribute(attributeName, upcarP->type, upcarP->contextValue);
  ContextElement*                 ceP           = new ContextElement();
  LM_M(("KZ: Attribute: %s:%s", attributeP->name.c_str(), attributeP->type.c_str()));

  // Copy the metadata vector of the input payload
  attributeP->metadataVector.fill((MetadataVector*) &upcarP->metadataVector);

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

  LM_M(("KZ: entityId='%s', attributeName='%s', ID='%s'", entityId.c_str(), attributeName.c_str(), valueId.c_str()));

  response.errorCode.code = SccNone;

  LM_M(("KZ: So, what is REALLY sent to mongoUpdateContext?"));
  LM_M(("KZ: ----------------------------------------------"));
  LM_M(("KZ: Entity id: '%s', type: '%s', isPattern: '%s', Attribute name: '%s', type: '%s', value: '%s', meta-ID: name: '%s', type: '%s', value: '%s'",
        request.contextElementVector[0]->entityId.id.c_str(), 
        request.contextElementVector[0]->entityId.type.c_str(), 
        request.contextElementVector[0]->entityId.isPattern.c_str(), 
        request.contextElementVector[0]->contextAttributeVector[0]->name.c_str(),
        request.contextElementVector[0]->contextAttributeVector[0]->type.c_str(),
        request.contextElementVector[0]->contextAttributeVector[0]->value.c_str(),
        request.contextElementVector[0]->contextAttributeVector[0]->metadataVector.get(0)->name.c_str(),
        request.contextElementVector[0]->contextAttributeVector[0]->metadataVector.get(0)->type.c_str(),
        request.contextElementVector[0]->contextAttributeVector[0]->metadataVector.get(0)->value.c_str()));

  ciP->httpStatusCode = mongoUpdateContext(&request, &response, ciP->tenant, ciP->servicePathV, ciP->uriParam, ciP->httpHeaders.xauthToken);

  LM_M(("KZ: mongoUpdateContext: %d", ciP->httpStatusCode));

  StatusCode statusCode;
  if (response.contextElementResponseVector.size() == 0)
  {
    LM_M(("KZ: contextElementResponseVector is empty"));
    statusCode.fill(SccContextElementNotFound,
                    std::string("Entity-Attribute pair: /") + entityId + "-" + attributeName + "/");
  }
  else if (response.contextElementResponseVector.size() == 1)
  {
    LM_M(("KZ: contextElementResponseVector of ONE element"));

    ContextElementResponse* cerP = response.contextElementResponseVector.get(0);

    LM_M(("KZ: cerP->statusCode.code == %d", cerP->statusCode.code));

    if (response.errorCode.code != SccNone)
    {
      LM_M(("KZ: response.errorCode.code == %d", response.errorCode.code));
      statusCode.fill(&response.errorCode);
    }
    else if (cerP->statusCode.code != SccNone)
    {
      LM_M(("KZ: cerP->statusCode.code == %d", cerP->statusCode.code));
      statusCode.fill(&cerP->statusCode);
    }
    else
    {
      LM_M(("KZ: no error/none"));
      statusCode.fill(SccOk);
    }
  }
  else
  {
    LM_M(("KZ: contextElementResponseVector of > 1 elements: ERROR"));
    statusCode.fill(SccReceiverInternalError,
                    "More than one response from putAttributeValueInstance::mongoUpdateContext");
  }

  request.release();
  LM_M(("KZ: statusCode.code == %d", statusCode.code));
  return statusCode.render(ciP->outFormat, "", false, false);
}
