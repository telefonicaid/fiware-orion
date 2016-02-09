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
* Author: Ken Zangelin
*/
#include "rapidjson/document.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseScope.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseScope - 
*/
std::string parseScope(ConnectionInfo* ciP, const Value::GenericValue* valueP, Scope* scopeP)
{
  std::string type  = jsonParseTypeNames[valueP->GetType()];

  if (type != "Object")
  {
    return "scope is not a JSON object";
  }

  if (!valueP->HasMember("type"))
  {
    return "scope without a type";
  }

  if (!valueP->HasMember("value"))
  {
    return "scope without a value";
  }

  for (Value::ConstMemberIterator iter = valueP->MemberBegin(); iter != valueP->MemberEnd(); ++iter)
  {
    std::string name  = iter->name.GetString();
    std::string type  = jsonParseTypeNames[iter->value.GetType()];

    if (name == "type")
    {
      if (type != "String")
      {
        return "invalid JSON type for scope type";
      }

      scopeP->type = iter->value.GetString();
    }
    else if (name == "value")
    {
      if (type != "String")
      {
        return "invalid JSON type for scope value";
      }

      scopeP->value = iter->value.GetString();
    }
    else
    {
      return std::string("unrecognized item in scope object: /") + name + "/";
    }
  }

  return "OK";
}
