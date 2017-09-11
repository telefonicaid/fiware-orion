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
* Author: Orion dev team
*/

#include "common/globals.h"
#include "common/JsonHelper.h"
#include "common/string.h"
#include "common/limits.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iomanip>


JsonHelper::JsonHelper(int _indent) : sb(), writer(sb), pwriter(sb) {
  indent = _indent;
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;    
  }
  pwriter.SetIndent(' ', indent);
}


void JsonHelper::Null() {
  if (indent == 0) {
    writer.Null();
  } else {
    pwriter.Null();
  }
}


void JsonHelper::Bool(bool value) {
  if (indent == 0) {
    writer.Bool(value);
  } else {
    pwriter.Bool(value);
  }
}


void JsonHelper::Key(const std::string& key) {
  if (indent == 0) {
    writer.Key(key.c_str());
  } else {
    pwriter.Key(key.c_str());
  }
}


void JsonHelper::Date(const std::string& key, long long value) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.String(isodate2str(value).c_str());
  } else {
    pwriter.Key(key.c_str());
    pwriter.String(isodate2str(value).c_str());
  }
}


void JsonHelper::Date(long long value) {
  if (indent == 0) {
    writer.String(isodate2str(value).c_str());
  } else {
    pwriter.String(isodate2str(value).c_str());
  }
}


void JsonHelper::Double(const std::string& key, double value) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.Double(value);
  } else {
    pwriter.Key(key.c_str());
    pwriter.Double(value);
  }
}


void JsonHelper::Double(double value) {
  if (indent == 0) {
    writer.Double(value);
  } else {
    pwriter.Double(value);
  }
}


void JsonHelper::Uint(const std::string& key, uint64_t value) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.Uint64(value);
  } else {
    pwriter.Key(key.c_str());
    pwriter.Uint64(value);
  }
}

void JsonHelper::Uint(uint64_t value) {
  if (indent == 0) {
    writer.Uint64(value);
  } else {
    pwriter.Uint64(value);
  }
}

void JsonHelper::Int(const std::string& key, int64_t value) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.Int64(value);
  } else {
    pwriter.Key(key.c_str());
    pwriter.Int64(value);
  }
}

void JsonHelper::Int(int64_t value) {
  if (indent == 0) {
    writer.Int64(value);
  } else {
    pwriter.Int64(value);
  }
}

void JsonHelper::String(const std::string& key, const std::string& value) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.String(value.c_str());
  } else {
    pwriter.Key(key.c_str());
    pwriter.String(value.c_str());
  }
}

void JsonHelper::String(const std::string& value) {
  if (indent == 0) {
    writer.String(value.c_str());
  } else {
    pwriter.String(value.c_str());
  }
}


void JsonHelper::StartArray(const std::string& key) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.StartArray();
  } else {
    pwriter.Key(key.c_str());
    pwriter.StartArray();
  }
}


void JsonHelper::StartArray() {
  if (indent == 0) {
    writer.StartArray();
  } else {
    pwriter.StartArray();
  }
}


void JsonHelper::StartObject(const std::string& key) {
  if (indent == 0) {
    writer.Key(key.c_str());
    writer.StartObject();
  } else {
    pwriter.Key(key.c_str());
    pwriter.StartObject();
  }
}

void JsonHelper::StartObject() {
  if (indent == 0) {
    writer.StartObject();
  } else {
    pwriter.StartObject();
  }
}


void JsonHelper::EndArray() {
  if (indent == 0) {
    writer.EndArray();
  } else {
    pwriter.EndArray();
  }
}


void JsonHelper::EndObject() {
  if (indent == 0) {
    writer.EndObject();
  } else {
    pwriter.EndObject();
  }
}


std::string JsonHelper::str() {
  return sb.GetString();
}



/* ****************************************************************************
*
* vectorToJson -
*/
template <>
void vectorToJson
(
  JsonHelper& writer,
  std::vector<std::string>& list
)
{
  writer.StartArray();

  for (std::vector<std::string>::size_type i = 0; i != list.size(); ++i)
  {
    writer.String(list[i]);
  }

  writer.EndArray();
}


/* ****************************************************************************
*
* objectToJson -
*/
void objectToJson
(
   JsonHelper& writer,
   std::map<std::string, std::string>& list
)
{
  writer.StartObject();

  for (std::map<std::string, std::string>::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    writer.Key(it->first);
    writer.String(it->first);
    writer.String(it->second);
  }

  writer.EndObject();
}

