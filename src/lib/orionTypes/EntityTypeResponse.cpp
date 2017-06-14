/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string>
#include <vector>

#include "rapidjson/prettywriter.h"

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "orionTypes/EntityTypeResponse.h"



/* ****************************************************************************
*
* EntityTypeResponse::render -
*/
void EntityTypeResponse::render
(
  rapidjson::Writer<rapidjson::StringBuffer>& writer,
  ApiVersion          apiVersion,
  bool                asJsonObject,
  bool                asJsonOut,
  bool                collapsed
)
{
  writer.StartObject();

  entityType.render(writer, apiVersion, asJsonObject, asJsonOut, collapsed, true);
  statusCode.render(writer);

  writer.EndObject();
}



/* ****************************************************************************
*
* EntityTypeResponse::check -
*/
std::string EntityTypeResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  bool                asJsonOut,
  bool                collapsed,
  const std::string&  predetectedError
)
{
  std::string res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = entityType.check(apiVersion, predetectedError)) != "OK")
  {
    alarmMgr.badInput(clientIp, res);
    statusCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
  writer.SetIndent(' ', 2);
  render(writer, apiVersion, asJsonObject, asJsonOut, collapsed);

  return s.GetString();
}



/* ****************************************************************************
*
* EntityTypeResponse::present -
*/
void EntityTypeResponse::present(const std::string& indent)
{
  LM_T(LmtPresent,("%sEntityTypeResponse:\n", indent.c_str()));
  entityType.present(indent + "  ");
  statusCode.present(indent + "  ");
}



/* ****************************************************************************
*
* EntityTypeResponse::release -
*/
void EntityTypeResponse::release(void)
{
  entityType.release();
  statusCode.release();
}



/* ****************************************************************************
*
* EntityTypeResponse::toJson -
*/
void EntityTypeResponse::toJson(rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
  writer.StartObject();

  writer.Key("attrs");
  writer.StartObject();
  entityType.contextAttributeVector.toJsonTypes(writer);
  writer.EndObject();

  writer.Key("count");
  writer.Uint(entityType.count);

  writer.EndObject();
}
