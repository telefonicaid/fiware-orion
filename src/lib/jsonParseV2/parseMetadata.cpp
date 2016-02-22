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
#include "rapidjson/document.h"

#include "logMsg/logMsg.h"

#include "orionTypes/OrionValueType.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Metadata.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseMetadata.h"



/* ****************************************************************************
*
* parseMetadataObject - 
*/
static std::string parseMetadataObject(const Value& start, Metadata* mP)
{
  for (Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
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

      mP->type      = iter->value.GetString();
      mP->typeGiven = true;
    }
    else if (name == "value")
    {
      if (type == "String")
      {
        mP->stringValue   = iter->value.GetString();
        mP->valueType     = orion::ValueTypeString;
      }
      else if (type == "Number")
      {
        mP->valueType     = orion::ValueTypeNumber;
        mP->numberValue   = iter->value.GetDouble();
      }
      else if (type == "True")
      {
        mP->valueType     = orion::ValueTypeBoolean;
        mP->boolValue     = true;
      }
      else if (type == "False")
      {
        mP->valueType     = orion::ValueTypeBoolean;
        mP->boolValue     = false;
      }
      else if (type == "Null")
      {
        mP->valueType     = orion::ValueTypeNone;
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

  return "OK";
}



/* ****************************************************************************
*
* parseMetadata - 
*
* Two options here:
*   "m1": "123"    (Value is  string or boolean or number)
*   "m1": { "value": "123", "type": "mt" }   (type is needed so a complex 'value' to the metadata)
*
*/
std::string parseMetadata(const Value& val, Metadata* mP)
{
  std::string type   = jsonParseTypeNames[val.GetType()];

  if (type == "Object")
  {
    return parseMetadataObject(val, mP);
  }

  mP->type = "";

  if (type == "String")
  {
    mP->stringValue  = val.GetString();
    mP->valueType    = orion::ValueTypeString;
  }
  else if (type == "Number")
  {
    mP->valueType    = orion::ValueTypeNumber;
    mP->numberValue  = val.GetDouble();
  }
  else if (type == "True")
  {
    mP->valueType    = orion::ValueTypeBoolean;
    mP->boolValue    = true;
  }
  else if (type == "False")
  {
    mP->valueType    = orion::ValueTypeBoolean;
    mP->boolValue    = false;
  }
  else if (type == "Null")
  {
    mP->valueType    = orion::ValueTypeNone;
  }
  else
  {
    alarmMgr.badInput(clientIp, "bad type for EntityId::ContextAttribute::Metadata");
    return "invalid JSON type for attribute metadata value";
  }

  return "OK";
}
