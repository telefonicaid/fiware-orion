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

#include "orionld/common/orionldState.h"        // orionldState

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "apiTypesV2/Subscription.h"
#include "common/JsonHelper.h"
#include "common/string.h"
#include "mongoBackend/mongoGetSubscriptions.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "common/idCheck.h"
#include "serviceRoutinesV2/getSubscription.h"



/* ****************************************************************************
*
* getSubscription -
*
* GET /v2/subscription/<id>
*
*/
std::string getSubscription
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  ngsiv2::Subscription sub;
  std::string          idSub = compV[2];
  OrionError           oe;
  std::string          out;
  std::string          err;

  if ((err = idCheck(idSub)) != "OK")
  {
    oe.fill(SccBadRequest, "Invalid subscription ID: " + err, "BadRequest");
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  TIMED_MONGO(mongoGetSubscription(&sub, &oe, idSub, ciP->uriParam, orionldState.tenantP));

  if (oe.code != SccOk)
  {
    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return out;
  }

  TIMED_RENDER(out = sub.toJson());
  return out;
}
