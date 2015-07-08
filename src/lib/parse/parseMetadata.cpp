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

#include "ngsi/Metadata.h"
#include "parse/jsonParseTypeNames.h"
#include "parse/parseMetadata.h"



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
        LM_E(("Bad Input (ContextAttribute::Metadata::type must be a String)"));
        return "Parse Error";
      }

      mP->type = iter->value.GetString();
    }
    else if (name == "value")
    {
      if (type == "String")
      {
        mP->value               = iter->value.GetString();
        mP->valueType           = MetadataValueTypeString;
        mP->valueValue.string   = (char*) mP->value.c_str();
      }
      else if (type == "Number")
      {
        mP->valueType           = MetadataValueTypeNumber;
        mP->valueValue.number   = iter->value.GetDouble();
      }
      else if (type == "True")
      {
        mP->valueType           = MetadataValueTypeBoolean;
        mP->valueValue.boolean  = true;
      }
      else if (type == "False")
      {
        mP->valueType           = MetadataValueTypeBoolean;
        mP->valueValue.boolean  = false;
      }
      else if (type == "Vector")
      {
        LM_E(("Bad Input (ContextAttribute::Metadata::type cannot be a Vector)"));
        return "Parse Error";
      }
      else if (type == "Object")
      {
        LM_E(("Bad Input (ContextAttribute::Metadata::type cannot be an Object)"));
        return "Parse Error";
      }
    }
    else
    {
      return "Parse Error";
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
  std::string name   = val.GetString();
  std::string type   = jsonParseTypeNames[val.GetType()];
  
  mP->name = name;

  if (type == "Object")
  {
    return parseMetadataObject(val, mP);
  }
  else if (type == "String")
  {
    mP->type               = "";
    mP->value              = val.GetString();
    mP->valueType          = MetadataValueTypeString;
    mP->valueValue.string  = (char*) val.GetString();
  }
  else if (type == "Number")
  {
    mP->type               = "";
    mP->valueType          = MetadataValueTypeNumber;
    mP->valueValue.number  = val.GetDouble();
  }
  else if (type == "True")
  {
    mP->type               = "";
    mP->valueType          = MetadataValueTypeBoolean;
    mP->valueValue.boolean = true;
  }
  else if (type == "False")
  {
    mP->type               = "";
    mP->valueType          = MetadataValueTypeBoolean;
    mP->valueValue.boolean = false;
  }
  else if (type == "Vector")
  {
    return "Parse Error";
  }
  else
  {
    LM_W(("Bad Input (bad type for EntityId::ContextAttribute::Metadata)"));
    return "Parse Error";
  }

  return "OK";
}
