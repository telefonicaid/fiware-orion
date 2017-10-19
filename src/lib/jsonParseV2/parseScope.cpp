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
#include <string>

#include "rapidjson/document.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseScope.h"

#include "common/errorMessages.h"


/* ****************************************************************************
*
* parseScopeValueLocation -
*
* Hint: "value": {
*          "georel": [ "near", "minDistance:13500" ],
*          "geometry": "point",
*          "coords": [ [40.418889,-3.691944] ]
*        }
*/
std::string parseScopeValueLocation(rapidjson::Value::ConstMemberIterator valueP, Scope* scopeP)
{
  // get value georel as a list of strings
  // get value geometry as a string
  // get coords as a list of pairs of numbers
  std::string georelS;
  std::string geometryS;
  std::string coordsS;

  rapidjson::Value::ConstMemberIterator iter;
  for (iter = valueP->value.MemberBegin(); iter != valueP->value.MemberEnd(); ++iter)
  {
    std::string name = iter->name.GetString();
    std::string type = jsonParseTypeNames[iter->value.GetType()];

    if (name == "georel")
    {
      if (type != "Array")
      {
        return "invalid JSON type for geometry (must be array)";
      }

      for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
      {
        std::string type = jsonParseTypeNames[iter2->GetType()];
        if (type != "String")
        {
          return "only JSON Strings allowed in geometry list";
        }

        georelS += iter2->GetString();
        if (iter2 != iter->value.End() - 1)
        {
          georelS += ";";
        }
      }
    }
    else if (name == "geometry")
    {
      if (type != "String")
      {
        return "invalid JSON type for geometry (must be string)";
      }

      geometryS = iter->value.GetString();
    }
    else if (name == "coords")
    {
      if (type != "Array")
      {
        return "invalid JSON type for geometry (must be array)";
      }

      for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
      {
        std::string type = jsonParseTypeNames[iter2->GetType()];

        if (type != "Array")
        {
          return "only JSON Arrays allowed in coords list";
        }

        rapidjson::Value::ConstValueIterator x = iter2->Begin();  // The first element in the array
        rapidjson::Value::ConstValueIterator y = x + 1;           // The next element in the array

        // Some sanity checks
        if (y != iter2->End() - 1)
        {
          return "coordinates array must have only 2 elements";
        }

        std::string typeX = jsonParseTypeNames[x->GetType()];
        std::string typeY = jsonParseTypeNames[y->GetType()];

        if (typeX != "Number" || typeY != "Number")
        {
          return "invalid JSON type for geometry (must be string)";
        }

        // Adding coord
        char buf[STRING_SIZE_FOR_DOUBLE * 2 + 2];

        snprintf(buf, sizeof(buf), "%f,%f", x->GetDouble(), y->GetDouble());
        coordsS += buf;

        if (iter2 != iter->value.End() - 1)
        {
          coordsS += ";";
        }
      }
    }
    else
    {
      return std::string("unrecognized item in value object: /") + name + "/";
    }
  }

  std::string result;
  scopeP->fill(V2, geometryS, coordsS, georelS, &result);

  return result;
}



/* ****************************************************************************
*
* parseScope - 
*/
std::string parseScope(ConnectionInfo* ciP, rapidjson::Value::ConstValueIterator valueP, Scope* scopeP)
{
  std::string type  = jsonParseTypeNames[valueP->GetType()];

  if (type != "Object")
  {
    return "scope is not a JSON object";
  }

  // Process scope type
  rapidjson::Value::ConstMemberIterator iter = valueP->FindMember("type");

  if (iter == valueP->MemberEnd())
  {
    return "scope without a type";
  }
  type = jsonParseTypeNames[iter->value.GetType()];

  if (type != "String")
  {
    return "invalid JSON type for scope type";
  }
  scopeP->type = iter->value.GetString();

  // Process scope value
  iter = valueP->FindMember("value");
  if (iter == valueP->MemberEnd())
  {
    return "scope without a value";
  }
  type = jsonParseTypeNames[iter->value.GetType()];

  // Processing depend on the filter type
  if (scopeP->type == FIWARE_LOCATION_V2)
  {
    if (type != "Object")
    {
      return "invalid JSON type for scope value (must be object)";
    }

    if (!iter->value.HasMember("georel"))
    {
      return "missing field in value: georel";
    }

    if (!iter->value.HasMember("geometry"))
    {
      return "missing field in value: geometry";
    }

    if (!iter->value.HasMember("coords"))
    {
      return "missing field in value: coords";
    }

    std::string r = parseScopeValueLocation(iter, scopeP);
    if (r != "")
    {
      return r;
    }
  }
  else   // Other filters (all them use 'value' as string)
  {
    if (type != "String")
    {
      return ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_SCOPE;
    }

    scopeP->value = iter->value.GetString();
  }

  if (scopeP->type == SCOPE_TYPE_SIMPLE_QUERY)
  {
    std::string errorString;

    scopeP->stringFilterP = new StringFilter(SftQ);
    bool b = scopeP->stringFilterP->parse(scopeP->value.c_str(), &errorString);

    if (b != true)
    {
      delete scopeP->stringFilterP;
      scopeP->stringFilterP = NULL;
      return errorString;
    }
  }
  else if (scopeP->type == SCOPE_TYPE_SIMPLE_QUERY_MD)
  {
    std::string errorString;

    scopeP->mdStringFilterP = new StringFilter(SftMq);
    bool b = scopeP->mdStringFilterP->parse(scopeP->value.c_str(), &errorString);

    if (b != true)
    {
      delete scopeP->mdStringFilterP;
      scopeP->mdStringFilterP = NULL;
      return errorString;
    }
  }

  return "OK";
}
