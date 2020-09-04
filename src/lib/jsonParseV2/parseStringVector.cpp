/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>
#include <algorithm>

#include "rapidjson/document.h"

#include "jsonParseV2/jsonParseTypeNames.h"



/* ****************************************************************************
*
* parseStringVector - 
*/
bool parseStringVector
(
  std::vector<std::string>*  sVecP,
  const rapidjson::Value&    jsonVector,
  const std::string&         fieldName,
  bool                       emptyStringNotAllowed,
  bool                       unique,
  std::string*               errorStringP
)
{
  std::string type = jsonParseTypeNames[jsonVector.GetType()];

  if (type != "Array")
  {
    *errorStringP = "the field /" + fieldName + "/ must be a JSON array";
    return false;
  }

  for (rapidjson::Value::ConstValueIterator iter = jsonVector.Begin(); iter != jsonVector.End(); ++iter)
  {
    std::string  val;

    type = jsonParseTypeNames[iter->GetType()];

    if (type != "String")
    {
      *errorStringP = "only JSON Strings allowed in string vector /" + fieldName + "/";
      return false;
    }

    std::string value = iter->GetString();

    if ((emptyStringNotAllowed == true) && (value.empty()))
    {
      *errorStringP = "empty string found in string vector /" + fieldName + "/";
      return false;
    }

    // If unique is true, we need to ensure the element hasn't been added previousy in order to add it
    if ((!unique) || (std::find(sVecP->begin(), sVecP->end(), value) == sVecP->end()))
    {
      sVecP->push_back(value);
    }
  }

  return true;
}
