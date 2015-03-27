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

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "rest/restReply.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/putAttributeValueInstance.h"



/* ****************************************************************************
*
* putAttributeValueInstance - 
*
* PUT /v1/contextEntities/{entity::id}/attributes/{attribute::name}/{metaID}
* PUT /ngsi10/contextEntities/{entity::id}/attributes/{attribute::name}/{metaID}
*
* Payload In:  UpdateContextAttributeRequest
* Payload Out: StatusCode
*
* URI parameters
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that don't use the standard operation QueryContext as there is no Restriction otherwise.
*     Here, entity::type can be empty though, and it is, unless the URI parameter 'entity::type=TYPE' is used.
*
* 01. Check validity of path components VS payload
* 02. Fill in UpdateContextRequest
* 03. Call postUpdateContext
* 04. Fill in StatusCode from UpdateContextResponse
* 05. Render result
* 06. Cleanup and return result
*/
std::string putAttributeValueInstance
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  entityId      = compV[2];
  std::string  attributeName = compV[4];
  std::string  metaID        = compV[5];
  std::string  entityType    = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string  answer;
  StatusCode   response;


  // 01. Check validity of path components VS payload
  Metadata* mP = parseDataP->upcar.res.metadataVector.lookupByName("ID");

  if ((mP != NULL) && (mP->value != metaID))
  {
    std::string details = "unmatching metadata ID value URI/payload: /" + metaID + "/ vs /" + mP->value + "/";
    
    response.fill(SccBadRequest, details);
    answer = response.render(ciP->outFormat, "", false, false);
    parseDataP->upcar.res.release();

    return answer;
  }


  // 02. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(&parseDataP->upcar.res, entityId, entityType, attributeName, metaID, "UPDATE");


  // 03. Call postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);


  // 04. Fill in StatusCode from UpdateContextResponse
  response.fill(parseDataP->upcrs.res);


  // 05. Render result
  answer = response.render(ciP->outFormat, "", false, false);


  // 06. Cleanup and return result
  response.release();
  parseDataP->upcar.res.release();
  parseDataP->upcr.res.release();
  parseDataP->upcrs.res.release();

  return answer;
}

#if 0
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

  ContextAttribute*               attributeP    = new ContextAttribute(attributeName, upcarP->type, upcarP->contextValue);
  ContextElement*                 ceP           = new ContextElement();

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

  response.errorCode.code = SccNone;

  ciP->httpStatusCode = mongoUpdateContext(&request, &response, ciP->tenant, ciP->servicePathV, ciP->uriParam, ciP->httpHeaders.xauthToken);

  StatusCode statusCode;
  if (response.contextElementResponseVector.size() == 0)
  {
    statusCode.fill(SccContextElementNotFound,
                    std::string("Entity-Attribute pair: /") + entityId + "-" + attributeName + "/");
  }
  else if (response.contextElementResponseVector.size() == 1)
  {
    ContextElementResponse* cerP = response.contextElementResponseVector.get(0);

    if (response.errorCode.code != SccNone)
    {
      statusCode.fill(&response.errorCode);
    }
    else if (cerP->statusCode.code != SccNone)
    {
      statusCode.fill(&cerP->statusCode);
    }
    else
    {
      statusCode.fill(SccOk);
    }
  }
  else
  {
    statusCode.fill(SccReceiverInternalError,
                    "More than one response from putAttributeValueInstance::mongoUpdateContext");
  }

  request.release();
  return statusCode.render(ciP->outFormat, "", false, false);
}

#endif
