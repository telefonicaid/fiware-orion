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
#include "common/tag.h"
#include "common/JsonHelper.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "serviceRoutinesV2/semStateTreat.h"
#include "alarmMgr/alarmMgr.h"
#include "mqtt/mqttMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "mongoDriver/mongoConnectionPool.h"



/* ****************************************************************************
*
* semRender - 
*
*/
static const std::string semRender(const char* state)
{
  JsonObjectHelper jh;

  jh.addString("status", state);

  //
  // FIXME P4 Fill in more fields here in the future (as part of issue #2145):
  //
  // "owner":     in case 'taken': who took it
  // "purpose":   in case 'taken': for what purpose
  // "errors":    number of errors for this semaphore
  // "waitTime":  total waiting-time for this semaphore
  // "count":     number of times the semaphore has been taken
  // "taken":     number of taken semaphores (for connectionEndpoints only)
  //

  return jh.str();
}



/* ****************************************************************************
*
* semStateTreat -
*/
std::string semStateTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  JsonObjectHelper jh;

  jh.addRaw("dbConnectionPool",    semRender(orion::mongoConnectionPoolSemGet()));
  jh.addRaw("dbConnection",        semRender(orion::mongoConnectionSemGet()));
  jh.addRaw("request",             semRender(reqSemGet()));
  jh.addRaw("subCache",            semRender(cacheSemGet()));
  jh.addRaw("transaction",         semRender(transSemGet()));
  jh.addRaw("timeStat",            semRender(timeStatSemGet()));
  jh.addRaw("logMsg",              semRender(lmSemGet()));
  jh.addRaw("alarmMgr",            semRender(alarmMgr.semGet()));
  jh.addRaw("mqttMgr",             semRender(mqttMgr.semGet()));
  jh.addRaw("metricsMgr",          semRender(metricsMgr.semStateGet()));
  jh.addRaw("connectionContext",   semRender(connectionSubContextSemGet()));
  jh.addRaw("connectionEndpoints", semRender(connectionSubContextSemGet()));

  return jh.str();
}
