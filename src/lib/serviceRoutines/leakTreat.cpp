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

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/rest.h"
#include "serviceRoutines/leakTreat.h"



/* ****************************************************************************
*
* leakTreat - 
*/
std::string leakTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string password = "XXX";
  std::string out;

  if (harakiri == false)
  {
    OrionError orionError(SccBadRequest, "no such service");

    ciP->httpStatusCode = SccOk;
    TIMED_RENDER(out = orionError.toJsonV1());
    return out;
  }

  if (components > 1)
  {
    password = compV[1];
  }

  if (components == 1)
  {
    OrionError orionError(SccBadRequest, "Password requested");
    ciP->httpStatusCode = SccOk;

    TIMED_RENDER(out = orionError.toJsonV1());
  }
  else if (password != "harakiri")
  {
    OrionError orionError(SccBadRequest, "Request denied - password erroneous");
    ciP->httpStatusCode = SccOk;

    TIMED_RENDER(out = orionError.toJsonV1());
  }
  else
  {
    // No Cleanup for valgrind, and just in case another malloc
    std::string pwd = strdup("Leak test done");
    OrionError orionError(SccOk, "Leak test: " + pwd);

    TIMED_RENDER(out = orionError.toJsonV1());
  }

  return out;
}
