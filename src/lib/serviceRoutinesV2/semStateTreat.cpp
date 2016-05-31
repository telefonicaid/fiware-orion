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

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "serviceRoutinesV2/semStateTreat.h"
#include "alarmMgr/alarmMgr.h"
#include "mongoBackend/mongoConnectionPool.h"



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
  //
  // Semaphores:
  //   - mongo connection pool
  //   - mongo connection
  //   - mongo request
  //   - subscription cache
  //   - transaction id
  //   - time statistics
  //   - log messages
  //   - alarm manager
  //   - curl x 2

  const char* mongoConnectionPoolSemState = mongoConnectionPoolSemGet();
  const char* mongoConnectionSemState     = mongoConnectionSemGet();
  const char* mongoRequestSemState        = reqSemGet();
  const char* subscriptionCacheSemState   = cacheSemGet();
  const char* transactionIdSemState       = transSemGet();
  const char* timeStatisticsSemState      = timeStatSemGet();
  const char* logMessageSemState          = lmSemGet();
  const char* alarmManagerSemState        = alarmMgr.semGet();
  const char* curl1SemState               = curl1SemGet();
  const char* curl2SemState               = curl2SemGet();

  std::string out = "{";

  out += JSON_STR("mongoConnectionPoolSem") + ":" + JSON_STR(mongoConnectionPoolSemState) + ",";
  out += JSON_STR("mongoConnectionSem")     + ":" + JSON_STR(mongoConnectionSemState) + ",";
  out += JSON_STR("mongoRequestSem")        + ":" + JSON_STR(mongoRequestSemState) + ",";
  out += JSON_STR("subscriptionCacheSem")   + ":" + JSON_STR(subscriptionCacheSemState) + ",";
  out += JSON_STR("transactionIdSem")       + ":" + JSON_STR(transactionIdSemState) + ",";
  out += JSON_STR("timeStatisticsSem")      + ":" + JSON_STR(timeStatisticsSemState) + ",";
  out += JSON_STR("logMessageSem")          + ":" + JSON_STR(logMessageSemState) + ",";
  out += JSON_STR("alarmManagerSem")        + ":" + JSON_STR(alarmManagerSemState) + ",";
  out += JSON_STR("curl1Sem")               + ":" + JSON_STR(curl1SemState) + ",";
  out += JSON_STR("curl2Sem")               + ":" + JSON_STR(curl2SemState);

  out += "}";

  return out;
}
