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

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "ngsi10/SubscribeContextResponse.h"

/* ****************************************************************************
*
* SubscribeContextResponse::~SubscribeContextResponse -
*/
SubscribeContextResponse::~SubscribeContextResponse() {
    LM_T(LmtDestructor,("destroyed"));
}

/* ****************************************************************************
*
* SubscribeContextResponse::SubscribeContextResponse - 
*/
SubscribeContextResponse::SubscribeContextResponse()
{
   subscribeError.errorCode.keyNameSet("errorCode");
   subscribeError.subscriptionId.set("000000000000000000000000");
}

/* ****************************************************************************
*
* SubscribeContextResponse::SubscribeContextResponse - 
*/
SubscribeContextResponse::SubscribeContextResponse(StatusCode& errorCode)
{
   subscribeError.subscriptionId.set("000000000000000000000000");
   subscribeError.errorCode.fill(&errorCode);
   subscribeError.errorCode.keyNameSet("errorCode");
}

/* ****************************************************************************
*
* SubscribeContextResponse::render - 
*/
std::string SubscribeContextResponse::render
(
  int indent
)
{
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;
  }

  writer.StartObject();

  if (subscribeError.errorCode.code == SccNone)
  {
    subscribeResponse.toJson(writer);
  }
  else
  {
    subscribeError.toJson(writer, SubscribeContext);
  }

  writer.EndObject();

  return sb.GetString();
}
