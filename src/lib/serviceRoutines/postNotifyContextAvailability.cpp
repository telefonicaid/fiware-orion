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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"
#include "common/defaultValues.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/mongoNotifyContextAvailability.h"
#include "ngsi/ParseData.h"
#include "ngsi9/NotifyContextAvailabilityRequest.h"
#include "ngsi9/NotifyContextAvailabilityResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postNotifyContextAvailability.h"



/* ****************************************************************************
*
* postNotifyContextAvailability -
*/
std::string postNotifyContextAvailability
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  NotifyContextAvailabilityResponse  ncar;
  std::string                        answer;

  //
  // If more than ONE service-path is input, an error is returned as response.
  // If NO service-path is issued, then the default service-path "/" is used.
  // After these checks, the service-path is checked to be 'correct'.
  //
  if (ciP->servicePathV.size() > 1)
  {
    ncar.responseCode.fill(SccBadRequest, "more than one service path for notification");
    alarmMgr.badInput(clientIp, "more than one service path for a notification");

    TIMED_RENDER(answer = ncar.toJsonV1());

    return answer;
  }
  else if (ciP->servicePathV.size() == 0)
  {
    ciP->servicePathV.push_back(SERVICE_PATH_ROOT);
  }

  std::string res = servicePathCheck(ciP->servicePathV[0].c_str());
  if (res != "OK")
  {
    ncar.responseCode.fill(SccBadRequest, res);

    TIMED_RENDER(answer = ncar.toJsonV1());

    return answer;
  }

  TIMED_MONGO(ciP->httpStatusCode = mongoNotifyContextAvailability(&parseDataP->ncar.res, &ncar, ciP->uriParam, ciP->httpHeaders.correlator, ciP->tenant, ciP->servicePathV[0]));
  TIMED_RENDER(answer = ncar.toJsonV1());

  return answer;
}
