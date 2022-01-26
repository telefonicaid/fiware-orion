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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldHeader.h"                       // orionldHeaderAdd

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "apiTypesV2/Registration.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "rest/HttpHeaders.h"                   // HTTP_*
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
  int                                offset = orionldState.uriParams.offset;
  int                                limit  = orionldState.uriParams.limit;
  long long                          count  = 0;

  TIMED_MONGO(mongoRegistrationsGet(&registrationV, orionldState.tenantP, ciP->servicePathV, offset, limit, &count, &oe));

  if (oe.code != SccOk)
  {
    TIMED_RENDER(out = oe.toJson());
    orionldState.httpStatusCode = oe.code;

    return out;
  }

  if (orionldState.uriParams.count)
    orionldHeaderAdd(&orionldState.out.headers, HttpNgsiv2Count, NULL, count);

  TIMED_RENDER(out = vectorToJson(registrationV));

  return out;
}
