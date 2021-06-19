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

#include "alarmMgr/alarmMgr.h"
#include "mongoBackend/mongoCreateSubscription.h"
#include "ngsi/ParseData.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "common/statistics.h"
#include "rest/HttpHeaders.h"
#include "rest/uriParamNames.h"
#include "rest/OrionError.h"
#include "orionld/common/orionldState.h"               // orionldState
#include "serviceRoutinesV2/postSubscriptions.h"



/* ****************************************************************************
*
* postSubscriptions -
*/
extern std::string postSubscriptions
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  SubscribeContextResponse  scr;
  std::string               answer = "";

  if (ciP->servicePathV.size() > 1)
  {
    const size_t  MSG_SIZE        = 96;  // strlen(msg) + enough room for digits
    char          errMsg[MSG_SIZE];
    ciP->httpStatusCode           = SccBadRequest;

    snprintf(errMsg, MSG_SIZE, "max *one* service-path allowed for subscriptions (%lu given)",
             (unsigned long) ciP->servicePathV.size());
    alarmMgr.badInput(clientIp, errMsg);
    scr.subscribeError.errorCode.fill(SccBadRequest, "max one service-path allowed for subscriptions");

    TIMED_RENDER(answer = scr.toJson());
    return answer;
  }

  OrionError  beError;
  std::string subsID;

  TIMED_MONGO(subsID = mongoCreateSubscription(parseDataP->subsV2,
                                               &beError,
                                               orionldState.tenantP,
                                               ciP->servicePathV,
                                               ciP->httpHeaders.xauthToken,
                                               ciP->httpHeaders.correlator,
                                               ""));

  // Check potential error
  if (beError.code != SccNone)
  {
    TIMED_RENDER(answer = beError.toJson());
    ciP->httpStatusCode = beError.code;
  }
  else
  {
    std::string location = "/v2/subscriptions/" + subsID;
    ciP->httpHeader.push_back(HTTP_RESOURCE_LOCATION);
    ciP->httpHeaderValue.push_back(location);

    ciP->httpStatusCode = SccCreated;
  }

  return answer;
}
