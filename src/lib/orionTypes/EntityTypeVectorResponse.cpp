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
#include "rapidjson/stringbuffer.h"

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "orionTypes/EntityTypeVectorResponse.h"



/* ****************************************************************************
*
* EntityTypeVectorResponse::render -
*/
void EntityTypeVectorResponse::render
(
  rapidjson::Writer<rapidjson::StringBuffer>& writer,
  ApiVersion          apiVersion,
  bool                asJsonObject,
  bool                asJsonOut,
  bool                collapsed
)
{
  writer.StartObject();

  if (entityTypeVector.size() > 0)
  {
    entityTypeVector.render(writer, apiVersion, asJsonObject, asJsonOut, collapsed);
  }

  statusCode.render(writer);

  writer.EndObject();
}



/* ****************************************************************************
*
* EntityTypeVectorResponse::check -
*/
std::string EntityTypeVectorResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  bool                asJsonOut,
  bool                collapsed,
  const std::string&  predetectedError)
{
  std::string res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = entityTypeVector.check(apiVersion, predetectedError)) != "OK")
  {
    alarmMgr.badInput(clientIp, res);
    statusCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
  writer.SetIndent(' ', 2);
  render(writer, apiVersion, asJsonObject, asJsonOut, collapsed);

  return s.GetString();
}



/* ****************************************************************************
*
* EntityTypeVectorResponse::present -
*/
void EntityTypeVectorResponse::present(const std::string& indent)
{
  LM_T(LmtPresent,("%s%d items in EntityTypeVectorResponse:\n",
		  indent.c_str(),
		  entityTypeVector.size()));

  entityTypeVector.present(indent + "  ");
  statusCode.present(indent + "  ");
}



/* ****************************************************************************
*
* EntityTypeVectorResponse::release -
*/
void EntityTypeVectorResponse::release(void)
{
  entityTypeVector.release();
  statusCode.release();
}


/* ****************************************************************************
*
* EntityTypeVectorResponse::toJson -
*/
void EntityTypeVectorResponse::toJson
(
  rapidjson::Writer<rapidjson::StringBuffer>& writer,
  bool values
)
{
  writer.StartArray();

  for (unsigned int ix = 0; ix < entityTypeVector.vec.size(); ++ix)
  {
    if (values)
    {
      writer.String(entityTypeVector.vec[ix]->type.c_str());
    }
    else  // default
    {
      entityTypeVector.vec[ix]->toJson(writer);
    }
  }

  writer.EndArray();
}
