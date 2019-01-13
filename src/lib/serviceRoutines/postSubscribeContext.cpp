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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/limits.h"
#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/mongoSubscribeContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/OrionError.h"
#include "serviceRoutines/postSubscribeContext.h"



/* ****************************************************************************
*
* postSubscribeContext - 
*
* POST /v1/subscribeContext
* POST /ngsi10/subscribeContext
*
* Payload In:  SubscribeContextRequest
* Payload Out: SubscribeContextResponse
*
*/
std::string postSubscribeContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  SubscribeContextResponse  scr;
  std::string               answer;
  bool skipInitialNotification = ciP->uriParamOptions[OPT_SKIPINITALNOTIFICATION];
  //
  // FIXME P0: Only *one* service path is allowed for subscriptions.
  //           Personally (kz) I kind of like that. If you want additional service-paths, just add another subscription!
  //           However, we need to at least state that HERE is where we limit the number of service paths to *one*.
  //
  if (ciP->servicePathV.size() > 1)
  {
    char  noOfV[STRING_SIZE_FOR_INT];
    snprintf(noOfV, sizeof(noOfV), "%lu", ciP->servicePathV.size());
    ciP->httpStatusCode           = SccOk;  // NGSIv1 is weird... it uses 200 OK at HTTP level for errors
    std::string details           = std::string("max *one* service-path allowed for subscriptions (") + noOfV + " given";

    alarmMgr.badInput(clientIp, details);

    scr.subscribeError.errorCode.fill(SccBadRequest, "max one service-path allowed for subscriptions");

    TIMED_RENDER(answer = scr.toJsonV1());
    return answer;
  }

  TIMED_MONGO(ciP->httpStatusCode = mongoSubscribeContext(&parseDataP->scr.res, &scr, ciP->tenant, ciP->httpHeaders.xauthToken, ciP->servicePathV, skipInitialNotification, ciP->httpHeaders.correlator));
  TIMED_RENDER(answer = scr.toJsonV1());

  parseDataP->scr.res.release();

  return answer;
}
