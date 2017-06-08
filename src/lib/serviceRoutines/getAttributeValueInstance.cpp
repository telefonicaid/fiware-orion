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

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "convenience/ContextAttributeResponse.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/getAttributeValueInstance.h"



/* ****************************************************************************
*
* getAttributeValueInstance -
*
* GET /ngsi10/contextEntities/{entityId::id}/attributes/{attribute::name}/{metadata::ID-value}
* GET /v1/contextEntities/{entityId::id}/attributes/{attribute::name}/{metadata::ID-value}
*
* Payload In:  None
* Payload Out: ContextAttributeResponse
*
* URI parameters:
*   - entity::type=TYPE
*   - exist=entity::type
*   - !exist=entity::type
*   - attributesFormat=object?
*
* 0. Take care of URI params
* 1. Fill in QueryContextRequest (includes adding URI parameters as Scope in restriction)
* 2. Call standard operation
* 3. Fill in ContextAttributeResponse from QueryContextResponse
* 4. If 404 Not Found - enter request entity data into response StatusCode::details
* 5. Render the ContextAttributeResponse
* 6. Cleanup and return result
*
* FIXME P1
* As the metadata ID matching is done outside mongoBackend, context provider forwarding
* is not supported. This problem could be solved adding the metadata ID value as a scope
* in the restriction of the QueryContext.
* Another option is to simply stop supporting this special metadata 'ID', however
* I believe the iotBroker of NEC depends on this ...
*/
std::string getAttributeValueInstance
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string               answer;
  std::string               entityId       = compV[2];
  std::string               entityType     = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string               attributeName  = compV[4];
  std::string               metaIdValue    = compV[5];
  EntityTypeInfo            typeInfo       = EntityTypeEmptyOrNotEmpty;
  ContextAttributeResponse  response;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  // 0. Take care of URI params
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 1. Fill in QueryContextRequest (includes adding URI parameters as Scope in restriction)
  parseDataP->qcr.res.fill(entityId, entityType, "false", typeInfo, attributeName);


  // 2. Call standard operation
  answer = postQueryContext(ciP, components, compV, parseDataP);


  //
  // 3. Fill in ContextAttributeResponse from QueryContextResponse.
  //    Special care with metadata named ID (and type 'xsd:string').
  //
  if (parseDataP->qcrs.res.contextElementResponseVector.size() != 0)
  {
    ContextElementResponse* cerP = parseDataP->qcrs.res.contextElementResponseVector[0];

    //
    // FIXME P4: as long as mongoQueryContext() signature is based on NGSI standard operations and that
    // standard queryContext doesn't allow specify metadata for attributes (note that it uses xs:string,
    // not full fledge attribute types), we cannot pass the ID to mongoBackend so we need to do the for loop
    // to grep the right attribute among all the ones returned by mongoQueryContext. However, this involves
    // a suboptimal query at mongoBackend, which could be improved passing it the ID as a new parameter to
    // mongoQueryContext() (although breaking the design principle about mongo*() functions follow the NGSI
    // standard). To think about ...
    //
    for (unsigned int i = 0; i < cerP->contextElement.contextAttributeVector.size(); i++)
    {
      if (cerP->contextElement.contextAttributeVector[i]->getId() == metaIdValue)
      {
        response.contextAttributeVector.push_back(cerP->contextElement.contextAttributeVector[i]);
      }
    }

    if (cerP->contextElement.contextAttributeVector.size() > 0 && response.contextAttributeVector.size() == 0)
    {
      response.statusCode.fill(SccContextElementNotFound,
                               std::string("Attribute-ValueID pair: /") + attributeName + "-" + metaIdValue + "/");
    }
    else
    {
      response.statusCode.fill(&cerP->statusCode);
    }
  }
  else
  {
    response.statusCode.fill(parseDataP->qcrs.res.errorCode);
  }


  // 4. If 404 Not Found - enter request entity data into response StatusCode::details
  if (response.statusCode.code == SccContextElementNotFound)
  {
    response.statusCode.details = "Entity id: /" + entityId + "/";
  }


  // 5. Render the ContextAttributeResponse
  TIMED_RENDER(answer = response.render(ciP->apiVersion, asJsonObject, IndividualContextEntityAttribute));


  // 6. Cleanup and return result
  // response.release();
  parseDataP->qcr.res.release();

  return answer;
}
