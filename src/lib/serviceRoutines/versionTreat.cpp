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

#include "common/string.h"
#include "common/globals.h"
#include "common/tag.h"
#include "common/compileInfo.h"
#include "common/defaultValues.h"
#include "rest/HttpHeaders.h"
#include "rest/rest.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/versionTreat.h"



/* ****************************************************************************
*
* version -
*/
static char versionString[30] = { 'a', 'l', 'p', 'h', 'a', 0 };



/* ****************************************************************************
*
* versionSet -
*/
void versionSet(const char* version)
{
  strncpy(versionString, version, sizeof(versionString));
}

/* ****************************************************************************
*
* versionGet -
*/
char* versionGet()
{
  return versionString;
}


/* ****************************************************************************
*
* versionTreat -
*/
std::string versionTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  if (isOriginAllowedForCORS(ciP->httpHeaders.origin))
  {
    ciP->httpHeader.push_back(ACCESS_CONTROL_ALLOW_ORIGIN);
    // If any origin is allowed, the header is always sent with the value "*"
    if (strcmp(corsOrigin, "__ALL") == 0)
    {
      ciP->httpHeaderValue.push_back("*");
    }
    // If a specific origin is allowed, the header is only sent if the origins match
    else
    {
      ciP->httpHeaderValue.push_back(corsOrigin);
    }
    ciP->httpHeader.push_back(ACCESS_CONTROL_EXPOSE_HEADERS);
    ciP->httpHeaderValue.push_back(CORS_EXPOSED_HEADERS);
  }
  
  std::string out     = "";
  std::string indent  = "";

#ifdef UNIT_TEST
  std::string uptime = "0 d, 0 h, 0 m, 0 s";
#else
  std::string uptime = parsedUptime(getTimer()->getCurrentTime() - startTime);
#endif

  out += "{\n";
  out += "\"orion\" : {\n";
  out += "  \"version\" : \"" + std::string(versionString) + "\",\n";
  out += "  \"uptime\" : \"" + std::string(uptime) + "\",\n";
  out += "  \"git_hash\" : \"" + std::string(GIT_HASH) + "\",\n";
  out += "  \"compile_time\" : \"" + std::string(COMPILE_TIME) + "\",\n";
  out += "  \"compiled_by\" : \"" + std::string(COMPILED_BY) + "\",\n";
  out += "  \"compiled_in\" : \"" + std::string(COMPILED_IN) + "\",\n";
  out += "  \"release_date\" : \"" + std::string(RELEASE_DATE) + "\",\n";
  out += "  \"doc\" : \"" + std::string(API_DOC) + "\"\n";
  out += "}\n";
  out += "}\n";

  ciP->httpStatusCode = SccOk;
  return out;
}
