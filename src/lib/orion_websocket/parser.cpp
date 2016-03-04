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

#include <map>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "logMsg/logMsg.h"
#include "rest/HttpHeaders.h"
#include "rest/ConnectionInfo.h"
#include "rest/rest.h"
#include "parser.h"

void ws_parser_parse
(
  const char*     msg,
  ConnectionInfo* ciP,
  std::string&    url,
  std::string&    verb,
  std::string&    payload,
  HttpHeaders&    head
)
{
  rapidjson::Document doc;


  doc.Parse(msg);

  if (!doc.IsObject())
  {
    url.clear();
    verb.clear();
    payload.clear();
    return;
  }

  url = doc["url"].GetString();
  verb = doc["verb"].GetString();

  if (doc.HasMember("payload") && doc["payload"].IsObject())
  {
    rapidjson::StringBuffer buff;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
    doc["payload"].Accept(writer);
    payload = buff.GetString();
  }
  else
    payload.clear();

  if (doc.HasMember("headers"))
  {
    std::map<std::string, std::string *>::iterator it = head.headerMap.begin();
    while (it != head.headerMap.end())
    {
      const char *value = doc["headers"][it->first.c_str()].GetString();
      *(it->second) = value ? std::string(value) : std::string();
      ++it;
    }

    head.gotHeaders = true;
  }
  else
  {
    head.gotHeaders = false;
  }

  if (doc.HasMember("params") && doc["params"].IsObject())
  {
    rapidjson::Value::ConstMemberIterator it = doc["params"].MemberBegin();
    while (it != doc["params"].MemberEnd())
    {
      uriParamPush(ciP, it->name.GetString(), it->value.GetString());
      ++it;
    }
  }
}

const char *ws_parser_message
(
 const std::string&  msg,
 const HttpHeaders&  head,
 int                 statusCode
)
{
  const char *tmpl = "{\"headers\": %s, \"message\": %s, \"status\": \"%d\"}";

  rapidjson::StringBuffer buff;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
  writer.StartObject();
  std::map<std::string, std::string *>::const_iterator it = head.headerMap.begin();
  while (it != head.headerMap.end())
  {
    writer.Key(it->first.c_str());
    writer.String(it->second->c_str());
    ++it;
  }
  writer.EndObject();

  if (statusCode < 100 || statusCode > 599) // Code is not a valid HTTP status code
    statusCode = 0;

  const char *headers = buff.GetString();

  /* Sum all lengths, discard string format specifier (-6)
   * and add status code (3) and the final null character
   */
  char *json = (char *) malloc(strlen(tmpl) - 6 + strlen(headers) + msg.size() + 3 + 1);
  sprintf(json, tmpl, headers, msg.c_str(), statusCode);

  return json;
}
