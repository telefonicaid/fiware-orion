/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rapidjson/document.h"

#include "utilsParse.h"

using namespace rapidjson;

using std::string;

/*
 * getStringAux
 */
static Opt<string> getStringAux(const Value& parent, const char* field, const string& description, bool optional) {
  if (parent.HasMember(field))
  {
    const Value& value = parent[field];
    if (!value.IsString())
    {
      return Opt<string>((!description.empty() ? description : field) + " is not a string");
    }

    return Opt<string>(value.GetString(), true);
  }
  else if (!optional)
  {
     return Opt<string>((!description.empty() ? description : field) + " is missing");
  }
  return Opt<string>("", false);
}



/*
 * getStringMust -  get a mandatory string from the rapidjson node
 */
extern Opt<string> getStringMust(const rapidjson::Value& parent, const char* field, const std::string& description) {
  return getStringAux(parent, field, description, false);
}



/*
 * getStringOpt - get a optional string from the rapidjson node
 */
extern Opt<string> getStringOpt(const rapidjson::Value& parent, const char* field, const std::string& description) {
  return  getStringAux(parent, field, description, true);
}



/*
 * getInt64Aux
 */
static Opt<int64_t> getInt64Aux(const Value& parent, const char* field, const string& description, bool optional) {
  if (parent.HasMember(field))
  {
    const Value& value = parent[field];
    if (!value.IsInt64())
    {
      return Opt<int64_t>((!description.empty() ? description : field) + " is not an int");
    }

    return Opt<int64_t>(value.GetInt64(), true);
  }
  else if (!optional)
  {
     return Opt<int64_t>((!description.empty() ? description : field) + " is missing");
  }
  return Opt<int64_t>(0, false);
}



/*
 * getInt64Must - get a mandatory int64_t from the rapidjson node
 */
extern Opt<int64_t> getInt64Must(const rapidjson::Value& parent, const char* field, const std::string& description) {
  return getInt64Aux(parent, field, description, false);
}



/*
 * getInt64Opt - get a optional int64_t from the rapidjson node
 */
extern Opt<int64_t> getInt64Opt(const rapidjson::Value& parent, const char* field, const std::string& description) {
  return  getInt64Aux(parent, field, description, true);
}

