/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms
* of the GNU Affero General Public License as
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
#include "common/JsonHelper.h"

std::string toJsonString(const std::string& input)
{
  std::ostringstream ss;
  ss << '"';
  for (std::string::const_iterator iter = input.begin(); iter != input.end(); ++iter) {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << *iter; break;
      }
  }
  ss << '"';
  return ss.str();
}

template <>
std::string vectorToJson(std::vector<std::string> &list)
{
  switch (list.size())
    {
    case 0:
      return "[]";
    case 1:
      return "[ " + toJsonString(list[0]) + " ]";
    default:
      std::ostringstream os;
      os << "[ ";
      os << toJsonString(list[0]);
      for(std::vector<std::string>::size_type i = 1; i != list.size(); ++i)
      {
          os << ", " << toJsonString(list[i]);
      }
      os << " ]";
      return os.str();
    }
}

JsonHelper::JsonHelper(): empty(true) {}

void JsonHelper::addString(const std::string& key, const std::string& value)
{
  if(!empty)
  {
    ss << ", ";
  }
  ss << toJsonString(key) << ": " << toJsonString(value);

  empty = false;
}

void JsonHelper::addRaw(const std::string& key, const std::string& value)
{
  if(!empty)
  {
    ss << ", ";
  }
  ss << toJsonString(key) << ": " << value;

  empty = false;
}

std::string JsonHelper::str()
{
  ss << "}";
  return "{" + ss.str();
}
