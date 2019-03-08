/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/defaultValues.h"
#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Registration.h"
#include "mongoBackend/mongoRegistrationCreate.h"
#include "serviceRoutinesV2/postRegistration.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* postRegistration - 
*
* POST /v2/registrations
*
* Payload In:  ngsiv2::Registration
* Payload Out: None
*/
std::string postRegistration
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  // FIXME P4: See issue #3078 about servicePath for registrations
  std::string           servicePath = (ciP->servicePathV[0] == "")? SERVICE_PATH_ROOT : ciP->servicePathV[0];
  ngsiv2::Registration  registration;
  OrionError            oe;
  std::string           regId;
  std::string           answer;

  //
  // FIXME P4: legacyForwardingMode = false to be implemented
  //
  if (parseDataP->reg.provider.legacyForwardingMode == false)
  {
    oe.fill(SccNotImplemented, "Only NGSIv1-based forwarding supported at the present moment. Set explictely legacyForwarding to true");
    ciP->httpStatusCode = oe.code;
    TIMED_RENDER(answer = oe.smartRender(ciP->apiVersion));
    return answer;
  }

  //
  // FIXME P4: Forwarding modes "none", "query", and "update" to be implemented
  //
  if (parseDataP->reg.provider.supportedForwardingMode != ngsiv2::ForwardAll)
  {
    oe.fill(SccNotImplemented, "non-supported Forwarding Mode");
    ciP->httpStatusCode = oe.code;
    TIMED_RENDER(answer = oe.smartRender(ciP->apiVersion));
    return answer;
  }

  TIMED_MONGO(mongoRegistrationCreate(&parseDataP->reg, ciP->tenant, servicePath, &regId, &oe));
  ciP->httpStatusCode = oe.code;

  if (oe.code != SccOk)
  {
    TIMED_RENDER(answer = oe.smartRender(ciP->apiVersion));
  }
  else
  {
    std::string location = "/v2/registrations/" + regId;

    ciP->httpHeader.push_back(HTTP_RESOURCE_LOCATION);
    ciP->httpHeaderValue.push_back(location);

    ciP->httpStatusCode = SccCreated;
  }

  return answer;
}
