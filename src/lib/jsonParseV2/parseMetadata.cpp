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
* Author: Ken Zangelin
*/
#include <string>

#include "rapidjson/document.h"

#include "logMsg/logMsg.h"

#include "orionTypes/OrionValueType.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Metadata.h"
#include "parse/CompoundValueNode.h"
#include "rest/OrionError.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseMetadataCompoundValue.h"
#include "jsonParseV2/parseMetadata.h"



/* ****************************************************************************
*
* parseMetadataObject -
*/
static std::string parseMetadataObject(const rapidjson::Value& start, Metadata* mdP)
{
  bool  compoundVector = false;

  // This is NGSIv2 parsing and in NGSIv2, no value means implicit null. Note that
  // valueTypeNotGiven will be overridden inside the 'for' block in case the attribute has an actual value
  mdP->valueType = orion::ValueTypeNull;

  for (rapidjson::Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "type")
    {
      if (type != "String")
      {
        alarmMgr.badInput(clientIp, "ContextAttribute::Metadata::type must be a String");
        return "invalid JSON type for attribute metadata type";
      }

      mdP->type      = iter->value.GetString();
      mdP->typeGiven = true;
    }
    else if (name == "value")
    {
      if (type == "String")
      {
        mdP->stringValue   = iter->value.GetString();
        mdP->valueType     = orion::ValueTypeString;
      }
      else if (type == "Number")
      {
        mdP->valueType     = orion::ValueTypeNumber;
        mdP->numberValue   = iter->value.GetDouble();
      }
      else if (type == "True")
      {
        mdP->valueType     = orion::ValueTypeBoolean;
        mdP->boolValue     = true;
      }
      else if (type == "False")
      {
        mdP->valueType     = orion::ValueTypeBoolean;
        mdP->boolValue     = false;
      }
      else if (type == "Null")
      {
        mdP->valueType     = orion::ValueTypeNull;
      }
      else if ((type == "Array") || (type == "Object"))
      {
        compoundVector = (type == "Array")? true : false;
        mdP->valueType = orion::ValueTypeObject;  // Used both for Array and Object ...
        std::string r  = parseMetadataCompoundValue(iter, mdP, NULL);

        if (r != "OK")
        {
          alarmMgr.badInput(clientIp, "json parse error in Metadata compound value");
          return "json parse error in Metadata compound value";
        }
      }
      else
      {
        std::string details = std::string("ContextAttribute::Metadata::type is '") + type + "'";
        alarmMgr.badInput(clientIp, details);
        return "invalid JSON type for attribute metadata value";
      }
    }
    else
    {
      alarmMgr.badInput(clientIp, "invalid JSON field for attribute metadata");
      return "invalid JSON field for attribute metadata";
    }
  }

  // Is it a date?
  if ((mdP->type == DATE_TYPE) || (mdP->type == DATE_TYPE_ALT))
  {
    mdP->numberValue =  parse8601Time(mdP->stringValue);

    if (mdP->numberValue == -1)
    {
      alarmMgr.badInput(clientIp, "date has invalid format");
      return "date has invalid format";
    }

    // Probably reseting stringValue is not needed, but let's do it for cleanliness
    mdP->stringValue = "";
    mdP->valueType   = orion::ValueTypeNumber;
  }

  if (!mdP->typeGiven)
  {
    mdP->type = (compoundVector)? defaultType(orion::ValueTypeVector) : defaultType(mdP->valueType);
  }

  return "OK";
}



/* ****************************************************************************
*
* parseMetadata -
*/
std::string parseMetadata(const rapidjson::Value& val, Metadata* mdP)
{
  std::string  type = jsonParseTypeNames[val.GetType()];
  std::string  s;

  if (type != "Object")
  {
    return "metadata must be a JSON object";
  }

  s = parseMetadataObject(val, mdP);

  return s;
}
