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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "apiTypesV2/Registration.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "mongoBackend/mongoRegistrationGet.h"  // FIXME P0: Two external functions in the same module ...
#include "alarmMgr/alarmMgr.h"
#include "serviceRoutinesV2/getRegistrations.h"



/* ****************************************************************************
*
* getRegistrations - 
*
* GET /v2/registrations
*
* Payload In:  None
* Payload Out: Vector of ngsiv2::Registration in JSON textual format
*
* URI parameters:
*   - limit=NUMBER
*   - offset=NUMBER
*   - count=true/false
* 
*/
std::string getRegistrations
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::vector<ngsiv2::Registration>  registrationV;
  OrionError                         oe;
  std::string                        out;
  int                                offset = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());
  int                                limit  = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());
  long long                          count  = 0;

  TIMED_MONGO(mongoRegistrationsGet(&registrationV, ciP->tenant, ciP->servicePathV, offset, limit, &count, &oe));

  if (oe.code != SccOk)
  {
    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;

    return out;
  }

  if ((ciP->uriParamOptions["count"]))
  {
    ciP->httpHeader.push_back("Fiware-Total-Count");
    ciP->httpHeaderValue.push_back(toString(count));
  }

  TIMED_RENDER(out = vectorToJson(registrationV));

  return out;
}
