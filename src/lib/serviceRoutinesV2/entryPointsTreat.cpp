/*
*
* Copyright (c) 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: José Manuel Cantera
*/
#include <string>
#include <vector>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/globals.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutinesV2/entryPointsTreat.h"


/* ****************************************************************************
*
* entryPointsTreat -
*/
std::string entryPointsTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  writer.SetIndent(' ', 2);

  writer.StartObject();
  writer.Key("entities_url");
  writer.String(ENTITIES_URL.c_str());
  writer.Key("types_url");
  writer.String(TYPES_URL.c_str());
  writer.Key("subscriptions_url");
  writer.String(SUBSCRIPTIONS_URL.c_str());
  writer.Key("registrations_url");
  writer.String(REGISTRATIONS_URL.c_str());
  writer.EndObject();

  ciP->httpStatusCode = SccOk;
  return sb.GetString();
}
