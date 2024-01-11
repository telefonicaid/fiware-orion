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
#include "logMsg/traceLevels.h"

#include "orionld/common/orionldState.h"                         // orionldState

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "serviceRoutinesV2/logLevelTreat.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* changeLogLevel -
*/
std::string changeLogLevel
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  if (orionldState.uriParams.level == NULL)
  {
    orionldState.httpStatusCode = SccBadRequest;
    alarmMgr.badInput(orionldState.clientIp, "no log level in URI param");
    return "{\"error\":\"log level missing\"}";
  }

  //
  // Internally, the broker does not support "warn", nor "fatal".
  // However, to ease the task for our users, the broker accepts both of them:
  //  - "fatal" is translated into the internal "None", meaning complete silence.
  //  - "warn" is translated into "Warning" which is accepted internally.
  //
  if ((strcasecmp(orionldState.uriParams.level, "none")    == 0) ||
      (strcasecmp(orionldState.uriParams.level, "fatal")   == 0) ||
      (strcasecmp(orionldState.uriParams.level, "error")   == 0) ||
      (strcasecmp(orionldState.uriParams.level, "warning") == 0) ||
      (strcasecmp(orionldState.uriParams.level, "warn")    == 0) ||
      (strcasecmp(orionldState.uriParams.level, "info")    == 0) ||
      (strcasecmp(orionldState.uriParams.level, "debug")   == 0))
  {
    if (strcasecmp(orionldState.uriParams.level, "warning") == 0)
      orionldState.uriParams.level = (char*) "WARN";

    lmLevelMaskSetString(orionldState.uriParams.level);
  }
  else
  {
    orionldState.httpStatusCode = SccBadRequest;
    alarmMgr.badInput(orionldState.clientIp, "invalid log level in URI param");
    return "{\"error\":\"invalid log level\"}";
  }

  return "";
}



/* ****************************************************************************
*
* getLogLevel -
*/
std::string getLogLevel
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  level = lmLevelMaskStringGet();

  return "{\"level\":\"" + level + "\"}";
}
