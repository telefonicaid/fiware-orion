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
* Author: Felipe Ortiz
*/

#ifndef WS_PARSER_H
#define WS_PARSER_H

#include <vector>
#include <map>
#include <string>

class HttpHeaders;
class ConnectionInfo;

void ws_parser_parse
(
  const char*      msg,
  ConnectionInfo*  ciP,
  std::string&     url,
  std::string&     verb,
  std::string&     payload,
  HttpHeaders&     head
);

const char *ws_parser_message
(
 const std::string&  msg,
 const HttpHeaders&  head,
 const std::vector<std::string> headName,
 const std::vector<std::string> headValue,
 int                 statusCode
);

const char *ws_parser_notify
(
 const std::string& subId,
 const std::map<std::string, std::string>& headers,
 const std::string& data
);

#endif
