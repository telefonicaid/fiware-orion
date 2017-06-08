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

#include "rest/ConnectionInfo.h"
#include "ngsi/AttributeList.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseAttributeList.h"



/* ****************************************************************************
*
* parseAttributeList - 
*/
std::string parseAttributeList
(
  ConnectionInfo*                               ciP,
  const rapidjson::Value::ConstMemberIterator&  iter,
  AttributeList*                                aP
)
{
  std::string type = jsonParseTypeNames[iter->value.GetType()];

  if (type != "Array")
  {
    return "the field /attributes/ must be a JSON array";
  }

  for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
  {
    std::string  val;

    type = jsonParseTypeNames[iter2->GetType()];

    if (type != "String")
    {
      return "only JSON Strings allowed in attribute list";
    }

    val  = iter2->GetString();
    aP->push_back(val);
  }

  return "OK";
}
