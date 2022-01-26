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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldHeader.h"                       // orionldHeaderAdd

#include "common/limits.h"                                     // STRING_SIZE_FOR_INT
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/rest.h"
#include "rest/HttpHeaders.h"                                  // CORS_EXPOSED_HEADERS, ...
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
  if (isOriginAllowedForCORS(orionldState.in.origin))
  {
    char* allowOrigin = (strcmp(corsOrigin, "__ALL") == 0)? (char*) "*" : corsOrigin;

    orionldHeaderAdd(&orionldState.out.headers, HttpAllowOrigin,   allowOrigin,                  0);
    orionldHeaderAdd(&orionldState.out.headers, HttpAllowMethods,  (char*) "GET, OPTIONS",       0);
    orionldHeaderAdd(&orionldState.out.headers, HttpExposeHeaders, (char*) CORS_EXPOSED_HEADERS, 0);
    orionldHeaderAdd(&orionldState.out.headers, HttpAllowHeaders,  (char*) CORS_ALLOWED_HEADERS, 0);
    orionldHeaderAdd(&orionldState.out.headers, HttpMaxAge,        NULL,                         corsMaxAge);
  }

  orionldState.httpStatusCode = SccOk;

  return "";
}
