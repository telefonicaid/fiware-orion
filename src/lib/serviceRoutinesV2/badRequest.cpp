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
#include "logMsg/traceLevels.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/restReply.h"
#include "serviceRoutinesV2/badRequest.h"



/* ****************************************************************************
*
* badRequest - 
*/
std::string badRequest
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  answer;
  std::string  details = std::string("service '") + ciP->url + "' not found";

  alarmMgr.badInput(ciP->ip, details);

  OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_SERVICE_NOT_FOUND);
  ciP->httpStatusCode = SccBadRequest;

  return oe.toJson();
}
