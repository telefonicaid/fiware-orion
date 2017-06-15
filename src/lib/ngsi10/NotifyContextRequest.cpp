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
* Author: Fermin Galan
*/
#include <string>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"


#include "common/globals.h"
#include "common/RenderFormat.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsi10/NotifyContextResponse.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* NotifyContextRequest::renderV1 -
*/
std::string NotifyContextRequest::renderV1
(
  bool       asJsonObject,
  int        indent
)
{
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;
  }
  writer.SetIndent(' ', indent);


  writer.StartObject();

  subscriptionId.toJson(writer, NotifyContext);
  originator.toJson(writer);
  contextElementResponseVector.toJsonV1(writer, asJsonObject, NotifyContext);

  writer.EndObject();

  return sb.GetString();
}



/* ****************************************************************************
*
* NotifyContextRequest::render -
*/
std::string NotifyContextRequest::render
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  const std::vector<std::string>&  metadataFilter,
  bool                             blacklist,
  int                              indent
)
{
  if ((renderFormat != NGSI_V2_NORMALIZED) && (renderFormat != NGSI_V2_KEYVALUES) && (renderFormat != NGSI_V2_VALUES))
  {
    OrionError oe(SccBadRequest, "Invalid notification format");
    alarmMgr.badInput(clientIp, "Invalid notification format");

    return oe.renderV1();
  }

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

  writer.StartObject();
  writer.Key("subscriptionId");
  writer.String(subscriptionId.get().c_str());
  writer.Key("data");
  writer.StartArray();

  contextElementResponseVector.toJson(writer, renderFormat, attrsFilter, metadataFilter, blacklist);
  writer.EndArray();
  writer.EndObject();

  return sb.GetString();
}



/* ****************************************************************************
*
* NotifyContextRequest::check
*/
std::string NotifyContextRequest::check(ApiVersion apiVersion, const std::string& indent, const std::string& predetectedError)
{
  std::string            res;
  NotifyContextResponse  response;

  if (predetectedError != "")
  {
    response.responseCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = subscriptionId.check(QueryContext, indent, predetectedError, 0))                    != "OK") ||
           ((res = originator.check(QueryContext, indent, predetectedError, 0))                        != "OK") ||
           ((res = contextElementResponseVector.check(apiVersion, QueryContext, indent, predetectedError, 0)) != "OK"))
  {
    response.responseCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render();
}



/* ****************************************************************************
*
* NotifyContextRequest::present -
*/
void NotifyContextRequest::present(const std::string& indent)
{
  subscriptionId.present(indent);
  originator.present(indent);
  contextElementResponseVector.present(indent);
}



/* ****************************************************************************
*
* NotifyContextRequest::release -
*/
void NotifyContextRequest::release(void)
{
  contextElementResponseVector.release();
}



/* ****************************************************************************
*
* NotifyContextRequest::clone -
*/
NotifyContextRequest* NotifyContextRequest::clone(void)
{
  NotifyContextRequest* ncrP = new NotifyContextRequest();

  ncrP->subscriptionId = subscriptionId;
  ncrP->originator     = originator;

  ncrP->contextElementResponseVector.fill(contextElementResponseVector);

  return ncrP;
}
