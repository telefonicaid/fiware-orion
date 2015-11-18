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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "apiTypesV2/Entities.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/OrionError.h"
#include "serviceRoutinesV2/postEntity.h"
#include "serviceRoutines/postUpdateContext.h"



/* ****************************************************************************
*
* postEntity -
*
* POST /v2/entities/{entityId}
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   op:    operation
*/
std::string postEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*      eP  = &parseDataP->ent.res;
  std::string  op  = ciP->uriParam["op"];

  eP->id = compV[2];

  if (op == "")
  {
    op = "APPEND";   // append or update
  }
  else if (op == "append") // pure-append
  {
    op = "APPEND_STRICT";
  }
  else
  {
    OrionError   error(SccBadRequest, "invalid value for URL parameter op");
    std::string  res;

    ciP->httpStatusCode = SccBadRequest;
    
    TIMED_RENDER(res = error.render(ciP, ""));

    return res;
  }

  // Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, op);

  // Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);

  // Any error in the response?
  UpdateContextResponse*  upcrsP = &parseDataP->upcrs.res;
  for (unsigned int ix = 0; ix < upcrsP->contextElementResponseVector.size(); ++ix)
  {
    if ((upcrsP->contextElementResponseVector[ix]->statusCode.code != SccOk) &&
        (upcrsP->contextElementResponseVector[ix]->statusCode.code != SccNone))
    {
      OrionError error(upcrsP->contextElementResponseVector[ix]->statusCode);
      std::string  res;

      ciP->httpStatusCode = error.code;
      TIMED_RENDER(res = error.render(ciP, ""));
      eP->release();

      return res;      
    }
  }

  // Default value for status code: SccCreated
  if ((ciP->httpStatusCode == SccOk) || (ciP->httpStatusCode == SccNone) || (ciP->httpStatusCode == SccCreated))
  {
    std::string location = "/v2/entities/" + eP->id;
    ciP->httpHeader.push_back("Location");
    ciP->httpHeaderValue.push_back(location);
    
    ciP->httpStatusCode = SccCreated;
  }

  // Cleanup and return result
  eP->release();

  return "";
}
