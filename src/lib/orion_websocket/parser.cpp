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
#include "parser.h"

void ws_parser_parse
(
    const char*   msg,
    std::string&  url,
    std::string&  verb,
    std::string&  payload,
    HttpHeaders&  head
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
    std::map<std::string, std::string *>::iterator it = head.supportedHeader.begin();
    while (it != head.supportedHeader.end())
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
}
