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

#include "orionld/types/ApiVersion.h"                     // ApiVersion
#include "orionld/common/orionldState.h"                  // orionldState

#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/rest.h"                                    // corsEnabled
#include "serviceRoutines/badVerbGetPostOnly.h"



/* ****************************************************************************
*
* badVerbGetPostOnly -
*/
std::string badVerbGetPostOnly
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  details = std::string("bad verb for url '") + orionldState.urlPath + "', method '" + orionldState.verbString + "'";
  char*        allowed;

  // OPTIONS verb is only available for V2 API
  if ((corsEnabled == true) && (orionldState.apiVersion == API_VERSION_NGSI_V2))  allowed = (char*) "POST, GET, OPTIONS";
  else                                                                            allowed = (char*) "POST, GET";

  orionldHeaderAdd(&orionldState.out.headers, HttpAllow, allowed, 0);
  orionldState.httpStatusCode = SccBadVerb;
  alarmMgr.badInput(orionldState.clientIp, details);

  if (orionldState.apiVersion == API_VERSION_NGSI_V1 || orionldState.apiVersion == API_VERSION_NONE)
    return "";

  OrionError oe(SccBadVerb, ERROR_DESC_BAD_VERB);
  return oe.smartRender(orionldState.apiVersion);
}
