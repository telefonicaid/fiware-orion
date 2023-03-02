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
* changeLogConfig -
*/
std::string changeLogConfig
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  level   = ciP->uriParam[URI_PARAM_LEVEL];
  const char*  levelP  = level.c_str();

  std::string logSize     = ciP->uriParam[URI_PARAM_LOG_SIZE];
  int logSizeValue        = atoi(logSize.c_str());
  std::string payloadSize = ciP->uriParam[URI_PARAM_PAYLOAD_SIZE];
  int payloadSizeValue    = atoi(payloadSize.c_str());

  if (level.empty() && logSize.empty() && payloadSize.empty())
  {
    ciP->httpStatusCode = SccBadRequest;
    alarmMgr.badInput(clientIp, "no log configs in URI param");
    return "{\"error\":\"log config missing\"}";
  }

  if (!logSize.empty())
  {
    if (logSizeValue > 0)
    {
      logLineMaxSize = logSizeValue;
    }
    else
    {
      ciP->httpStatusCode = SccBadRequest;
      alarmMgr.badInput(clientIp, "invalid logLineMaxSize in URI param", logSize);
      return "{\"error\":\"invalid logLineMaxSize, logLine size should be > 0\"}";
    }
  }

  if (!payloadSize.empty())
  {
    if (payloadSizeValue > 0)
    {
      logInfoPayloadMaxSize = payloadSizeValue;
    }
    else if (payloadSizeValue < 0)
    {
      ciP->httpStatusCode = SccBadRequest;
      alarmMgr.badInput(clientIp, "invalid logInfoPayloadMaxSize in URI param", payloadSize);
      return "{\"error\":\"invalid logInfoPayloadMaxSize,logPayload size should be > 0\"}";
    }
  }

  //
  // Internally, the broker does not support "warn", nor "fatal".
  // However, to easy the task for our users, the broker accepts both of them:
  //  - "fatal" is translated into the internal "None", meaning complete silence.
  //  - "warn" is translated into "Warning" which is accepted internally.
  //
  if (!level.empty())
  {
    if ((strcasecmp(levelP, "none")    == 0) ||
        (strcasecmp(levelP, "fatal")   == 0) ||
        (strcasecmp(levelP, "error")   == 0) ||
        (strcasecmp(levelP, "warning") == 0) ||
        (strcasecmp(levelP, "warn")    == 0) ||
        (strcasecmp(levelP, "info")    == 0) ||
        (strcasecmp(levelP, "debug")   == 0))
    {
      if (strcasecmp(levelP, "warning") == 0)
      {
        level = "WARN";
      }

      lmLevelMaskSetString((char*) level.c_str());
    }
    else
    {
      ciP->httpStatusCode = SccBadRequest;
      alarmMgr.badInput(clientIp, "invalid log level in URI param", level);
      return "{\"error\":\"invalid log level\"}";
    }
  }

  return "";
}



/* ****************************************************************************
*
* getLogConfig -
*/
std::string getLogConfig
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  level = lmLevelMaskStringGet();
  std::string  payloadMaxSize = std::to_string(logInfoPayloadMaxSize);
  std::string  lineMaxSize    = std::to_string(logLineMaxSize);

  return "{\"level\":\"" + level + "\", \
           \"infoPayloadMaxSize\":\"" + payloadMaxSize + "\", \
           \"lineMaxSize\":\"" + lineMaxSize + "\"}";
}
