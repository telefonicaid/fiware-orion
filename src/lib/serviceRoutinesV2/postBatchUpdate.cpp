/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "apiTypesV2/ErrorCode.h"
#include "apiTypesV2/Entities.h"
#include "serviceRoutinesV2/postBatchUpdate.h"
#include "ngsi10/UpdateContextRequest.h"
#include "serviceRoutines/postUpdateContext.h"



/* ****************************************************************************
*
* postBatchUpdate -
*
* POST /v2/op/update
*
* Payload In:  BatchUpdateRequest
* Payload Out: 201 or error
*
* URI parameters:
*   - limit=NUMBER
*   - offset=NUMBER
*   - options=keyValues
*/
std::string postBatchUpdate
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  BatchUpdate*           buP    = &parseDataP->bu.res;
  UpdateContextRequest*  upcrP  = &parseDataP->upcr.res;
  Entities               entities;
  std::string            answer;

  upcrP->fill(&buP->entities, buP->updateActionType.get());
  buP->release();  // upcrP just 'took over' the data from buP, buP is no longer needed
  parseDataP->upcr.res.present("");
  answer = postUpdateContext(ciP, components, compV, parseDataP);

#if 0
  for (unsigned int ix = 0; ix < parseDataP->upcrs.res.contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP = parseDataP->upcrs.res.contextElementResponseVector[ix];

    if (cerP->statusCode.code != SccOk)
    {
      parseDataP->upcrs.res.errorCode.fill(cerP->statusCode);
    }
  }


  //
  // If an error is flagged by ciP->httpStatusCode, store it in parseDataP->upcrs.res.errorCode
  // for later processing (not sure this ever happen ...)
  //
  if (ciP->httpStatusCode != SccOk)
  {
    parseDataP->upcrs.res.errorCode.code     = ciP->httpStatusCode;
    parseDataP->upcrs.res.errorCode.details  = answer;
  }

  // If postUpdateContext gives back a parseDataP->upcrs with !200 OK in 'errorCode', transform to HTTP Status error
  if ((parseDataP->upcrs.res.errorCode.code != SccOk) && (parseDataP->upcrs.res.contextElementResponseVector.size() == 1))
  {
    OrionError   oe(parseDataP->upcrs.res.errorCode);

    ciP->httpStatusCode = parseDataP->upcrs.res.errorCode.code;

    // If 404 and details empty, assuming 'Entity not found'
    if ((parseDataP->upcrs.res.errorCode.code == SccContextElementNotFound) && (oe.details == ""))
    {
      oe.details = "Entity not found";
    }

    answer = oe.render(ciP, "");
  }
  else
  {
    //
    // NOTE
    //   For simplicity, 204 is always returned, even if entities are created
    //
    ciP->httpStatusCode = SccNoContent;
    answer = "";
  }
#else
  if (parseDataP->upcrs.res.oe.code != SccNone )
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.oe.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.oe.code;
  }
  else
  {
    answer = "";
    ciP->httpStatusCode = SccNoContent;
  }
#endif

  // 04. Cleanup and return result
  entities.release();
  parseDataP->upcr.res.release();

  return answer;
}
