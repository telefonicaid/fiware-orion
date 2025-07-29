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
#include "common/errorMessages.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "rest/OrionError.h"
#include "serviceRoutinesV2/postEntities.h"
#include "serviceRoutinesV2/postUpdateContext.h"

static const int STRUCTURAL_OVERHEAD_BSON_ID = 10;



/* ****************************************************************************
*
* legalEntityLength -
*
* Check if the entity length is supported by Mongo
*/

static bool legalEntityLength(Entity* eP, const std::string& servicePath)
{
  return (servicePath.size() + eP->entityId.id.size() + eP->entityId.type.size() + STRUCTURAL_OVERHEAD_BSON_ID) < 1024;
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
*   options=keyValues
*   options=upsert
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
  Entity*   eP = &parseDataP->ent.res;
  bool  upsert = ciP->uriParamOptions[OPT_UPSERT];

  if (!legalEntityLength(eP, ciP->httpHeaders.servicePath))
  {
    OrionError oe(SccBadRequest, "Too long entity id/type/servicePath combination", ERROR_BAD_REQUEST);
    eP->release();

    std::string out;
    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;

    return out;
  }

  // Set some aspects depending on upsert or not upsert
  ActionType      actionType;
  Ngsiv2Flavour   ngsiv2flavour;
  HttpStatusCode  sccCodeOnSuccess;
  if (upsert)
  {
    actionType       = ActionTypeAppend;
    ngsiv2flavour    = NGSIV2_NO_FLAVOUR;
    sccCodeOnSuccess = SccNoContent;
  }
  else
  {
    actionType       = ActionTypeAppendStrict;
    ngsiv2flavour    = NGSIV2_FLAVOUR_ONCREATE;
    sccCodeOnSuccess = SccCreated;
  }

  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, actionType);

  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP, ngsiv2flavour);

  //
  // 03. Check error
  //
  std::string  answer = "";
  if (parseDataP->upcrs.res.error.code != SccOk)
  {
    ciP->httpStatusCode = parseDataP->upcrs.res.error.code;
    TIMED_RENDER(answer = parseDataP->upcrs.res.error.toJson());
    ciP->answer         = answer;
  }
  else
  {
    // Prepare HTTP headers
    std::string location = "/v2/entities/" + eP->entityId.id;
    if (!eP->entityId.type.empty())
    {
      location += "?type=" + eP->entityId.type;
    }
    else
    {
      location += "?type=none";
    }

    ciP->httpHeader.push_back(HTTP_RESOURCE_LOCATION);
    ciP->httpHeaderValue.push_back(location);
    ciP->httpStatusCode = sccCodeOnSuccess;
  }

  // 04. Cleanup and return result
  eP->release();

  return answer;
}
