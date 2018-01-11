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
* Author: Burak Karaboga - ATOS Research & Innovation
*/
#include <string>
#include <vector>

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "rest/rest.h"
#include "serviceRoutinesV2/optionsPostOnly.h"



/* ****************************************************************************
*
* optionsPostOnly -
* Adds HTTP header Access-Control-Allow-Methods to the response
* with the value: "POST, OPTIONS"
*/
std::string optionsPostOnly
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  if (isOriginAllowedForCORS(ciP->httpHeaders.origin))
  {
    ciP->httpHeader.push_back(ACCESS_CONTROL_ALLOW_METHODS);
    ciP->httpHeaderValue.push_back("POST, OPTIONS");
  }
  ciP->httpStatusCode = SccOk;

  return "";
}
