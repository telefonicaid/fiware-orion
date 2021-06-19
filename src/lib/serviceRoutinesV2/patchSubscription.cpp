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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "orionld/common/orionldState.h"             // orionldState

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "mongoBackend/mongoUpdateSubscription.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"
#include "serviceRoutinesV2/patchSubscription.h"



/* ****************************************************************************
*
* patchSubscription -
*
* PATCH /v2/subscriptions/{entityId}
*
* Payload In:  None
* Payload Out: None
*
* URI parameters:
*   -
*/
std::string patchSubscription
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  subscriptionId = compV[2];

  // 'Fill In' SusbcriptionUpdate
  parseDataP->subsV2.id = subscriptionId;


  OrionError beError;
  //
  // If a string-filter is present, it is parsed in
  // jsonParseV2/parseSubscription.cpp, function parseNotifyConditionVector() and
  // the resulting StringFilter object resides in a Scope in parseDataP->subsV2.restriction.scopeVector
  //
  TIMED_MONGO(mongoUpdateSubscription(parseDataP->subsV2,
                                      &beError,
                                      orionldState.tenantP,
                                      ciP->servicePathV,
                                      ciP->httpHeaders.xauthToken,
                                      ciP->httpHeaders.correlator));

  std::string  answer = "";
  if (beError.code != SccNone)
  {
    TIMED_RENDER(answer = beError.toJson());
    ciP->httpStatusCode = beError.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  return answer;
}
