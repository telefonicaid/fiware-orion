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

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/rest.h"
#include "rest/restReply.h"
#include "serviceRoutines/exitTreat.h"
#include "cache/subCache.h"



/* ****************************************************************************
*
* exitTreat -
*
* FIXME P5: this function maybe is no longer needed. The only known user was the exit logic
* in valgrind test pass (functional test has been using SIGTERM since a long term or maybe
* always) that has changed to SIGTERM due to issue https://github.com/telefonicaid/fiware-orion/issues/3809
*/
std::string exitTreat
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
    out = orionError.toJsonV1();
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
    out = orionError.toJsonV1();
  }
  else if (password != "harakiri")
  {
    OrionError orionError(SccBadRequest, "Request denied - password erroneous");
    ciP->httpStatusCode = SccOk;
    out = orionError.toJsonV1();
  }
  else
  {
    compV.clear();
    return "DIE";
  }

  return out;
}
