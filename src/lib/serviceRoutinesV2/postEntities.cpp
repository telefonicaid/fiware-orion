/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/EntityTypeInfo.h"
#include "rest/OrionError.h"
#include "serviceRoutinesV2/postEntities.h"
#include "serviceRoutines/postUpdateContext.h"

static const int STRUCTURAL_OVERHEAD_BSON_ID = 10;



/* ****************************************************************************
*
* legalEntityLength -
*
* Check if the entity length is supported by Mongo
*/

static bool legalEntityLength(Entity* eP, const std::string& servicePath)
{
  return (servicePath.size() + eP->id.size() + eP->type.size() + STRUCTURAL_OVERHEAD_BSON_ID) < 1024;
}



/* ****************************************************************************
*
* postEntities - 
*
* POST /v2/entities
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   - 
*
* 01. Fill in UpdateContextRequest
* 02. Call standard op postUpdateContext
* 03. Prepare HTTP headers
* 04. Cleanup and return result
*/
std::string postEntities
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*  eP = &parseDataP->ent.res;

  if (!legalEntityLength(eP, ciP->servicePath))
  {
    OrionError oe(SccBadRequest, "Too long entity id/type/servicePath combination");
    ciP->httpStatusCode = SccBadRequest;
    eP->release();

    std::string out;
    TIMED_RENDER(out = oe.render(ciP, ""));

    return out;
  }

  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, "APPEND_STRICT");
  

  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP, true);

  StatusCode     rstatuscode = parseDataP->upcrs.res.contextElementResponseVector[0]->statusCode;
  HttpStatusCode rhttpcode  = rstatuscode.code;
  std::string    answer;

  // 03. Prepare HTTP headers
  if (rhttpcode == SccOk || rhttpcode == SccNone)
  {
    std::string location = "/v2/entities/" + eP->id;

    ciP->httpHeader.push_back("Location");
    ciP->httpHeaderValue.push_back(location);
    ciP->httpStatusCode = SccCreated;
  }
  else if (rhttpcode == SccInvalidModification)
  {
    OrionError oe(rstatuscode);
    ciP->httpStatusCode = SccInvalidModification;

    TIMED_RENDER(answer = oe.render(ciP, ""));
  }


  // 04. Cleanup and return result
  eP->release();

  return answer;
}
