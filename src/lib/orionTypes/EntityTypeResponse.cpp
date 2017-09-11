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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "orionTypes/EntityTypeResponse.h"



/* ****************************************************************************
*
* EntityTypeResponse::render -
*/
std::string EntityTypeResponse::render
(
  ApiVersion apiVersion,
  bool       asJsonObject,
  bool       asJsonOut,
  bool       collapsed,
  int        indent
)
{
  JsonHelper writer(indent);

  toJson(writer, apiVersion, asJsonObject, asJsonOut, collapsed);

  return writer.str();
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

  return render(apiVersion, asJsonObject, asJsonOut, collapsed);
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
void EntityTypeResponse::toJson
(
  JsonHelper&  writer,
  ApiVersion   apiVersion,
  bool         asJsonObject,
  bool         asJsonOut,
  bool         collapsed
)
{
  writer.StartObject();

  if (apiVersion == V2)
  {
    writer.StartObject("attrs");
    entityType.contextAttributeVector.toJsonTypes(writer);
    writer.EndObject();

    writer.Int("count", entityType.count);
  }
  else
  {
    entityType.toJsonV1(writer, apiVersion, asJsonObject, asJsonOut, collapsed, true);
    statusCode.toJsonV1(writer);
  }

  writer.EndObject();
}
