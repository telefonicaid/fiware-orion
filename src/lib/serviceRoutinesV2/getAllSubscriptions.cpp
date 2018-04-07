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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/JsonHelper.h"
#include "common/string.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/mongoGetSubscriptions.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "serviceRoutinesV2/getAllSubscriptions.h"



/* ****************************************************************************
*
* getAllSubscriptions -
*
* GET /v2/subscriptions
*
*/
std::string getAllSubscriptions
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::vector<ngsiv2::Subscription> subs;
  OrionError                        oe;
  long long                         count  = 0;
  int                               offset = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());
  int                               limit  = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());

  TIMED_MONGO(mongoListSubscriptions(&subs,
                                     &oe,
                                     ciP->uriParam,
                                     ciP->tenant,
                                     ciP->servicePathV[0],
                                     limit,
                                     offset,
                                     &count));

  if (oe.code != SccOk)
  {
    std::string out;

    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;

    return out;
  }

  if ((ciP->uriParamOptions["count"]))
  {
    ciP->httpHeader.push_back("Fiware-Total-Count");
    ciP->httpHeaderValue.push_back(toString(count));
  }

  std::string out;
  TIMED_RENDER(out = vectorToJson(subs));

  return out;
}
