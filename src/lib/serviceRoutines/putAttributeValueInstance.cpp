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

  if ((mP != NULL) && (mP->stringValue != metaID))
  {
    std::string details = "unmatching metadata ID value URI/payload: /" + metaID + "/ vs /" + mP->stringValue + "/";

    response.fill(SccBadRequest, details);

    TIMED_RENDER(answer = response.render(false, false));

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
  TIMED_RENDER(answer = response.render(false, false));


  // 06. Cleanup and return result
  response.release();
  parseDataP->upcar.res.release();
  parseDataP->upcr.res.release();
  parseDataP->upcrs.res.release();

  return answer;
}
