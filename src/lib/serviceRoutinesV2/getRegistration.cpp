/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "orionld/common/orionldState.h"

#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Registration.h"
#include "mongoBackend/mongoRegistrationGet.h"
#include "serviceRoutinesV2/getRegistration.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* getRegistration - 
*
* GET /v2/registrations/<reg id>
*
* Payload In:  None
* Payload Out: ngsiv2::Registration in JSON textual format
*/
std::string getRegistration
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string           regId  = compV[2];
  ngsiv2::Registration  registration;
  OrionError            oe;

  TIMED_MONGO(mongoRegistrationGet(&registration, regId, orionldState.tenantP, ciP->servicePathV[0], &oe));

  if (oe.code != SccOk)
  {
    return oe.toJson();
  }

  return registration.toJson();
}
