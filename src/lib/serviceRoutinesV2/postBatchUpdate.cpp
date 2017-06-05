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
#include <vector>

#include "logMsg/logMsg.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "apiTypesV2/Entities.h"
#include "serviceRoutinesV2/postBatchUpdate.h"
#include "ngsi10/UpdateContextRequest.h"
#include "serviceRoutines/postUpdateContext.h"

#include "alarmMgr/alarmMgr.h"



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

  upcrP->fill(&buP->entities, buP->updateActionType.get());
  buP->release();  // upcrP just 'took over' the data from buP, buP is no longer needed
  parseDataP->upcr.res.present("");

  std::string  answer = "";
  if (parseDataP->upcr.res.contextElementVector.size() == 0)
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITIES_VECTOR);
    alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITIES_VECTOR);

    TIMED_RENDER(answer = oe.smartRender(V2));
    ciP->httpStatusCode = SccBadRequest;

    return answer;
  }

  postUpdateContext(ciP, components, compV, parseDataP);

  // Check potential error
  if (parseDataP->upcrs.res.oe.code != SccNone )
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.oe.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.oe.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  // Cleanup and return result
  entities.release();
  parseDataP->upcr.res.release();

  return answer;
}
