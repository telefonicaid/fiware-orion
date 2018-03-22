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
* Author: Burak Karaboga - ATOS Research & Innovation
*/
#include <string>
#include <vector>

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "rest/rest.h"
#include "serviceRoutines/optionsVersionRequest.h"



/* ****************************************************************************
*
* optionsVersionRequest -
* Special handler for OPTIONS requests on /version
* Adds all CORS related headers to the response
*/
std::string optionsVersionRequest
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

    ciP->httpHeader.push_back(ACCESS_CONTROL_ALLOW_METHODS);
    ciP->httpHeaderValue.push_back("GET, OPTIONS");

    ciP->httpHeader.push_back(ACCESS_CONTROL_EXPOSE_HEADERS);
    ciP->httpHeaderValue.push_back(CORS_EXPOSED_HEADERS);

    ciP->httpHeader.push_back(ACCESS_CONTROL_ALLOW_HEADERS);
    ciP->httpHeaderValue.push_back(CORS_ALLOWED_HEADERS);

    char maxAge[STRING_SIZE_FOR_INT];
    snprintf(maxAge, sizeof(maxAge), "%d", corsMaxAge);
    ciP->httpHeader.push_back(ACCESS_CONTROL_MAX_AGE);
    ciP->httpHeaderValue.push_back(maxAge);
  }

  ciP->httpStatusCode = SccOk;

  return "";
}