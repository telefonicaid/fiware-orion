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

#include "ngsi/ContextAttribute.h"
#include "parse/jsonParseTypeNames.h"
#include "parse/parseContextAttribute.h"
#include "parse/parseMetadata.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseContextAttributeObject - 
*/
static std::string parseContextAttributeObject(const Value& start, ContextAttribute* caP)
{
  for (Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "type")
    {
      if (type != "String")
      {
        LM_E(("Bad Input (ContextAttribute::Object::type must be a String"));
        return "Parse Error";
      }

      caP->type = iter->value.GetString();
    }
    else if (name == "value")
    {
      if (type == "String")
      {
        caP->value             = iter->value.GetString();
        caP->valueType         = ValueTypeString;
        caP->valueValue.string = (char*) caP->value.c_str();
      }
      else if (type == "Number")
      {
        caP->valueType          = ValueTypeNumber;
        caP->valueValue.number  = iter->value.GetDouble();
      }
      else if (type == "True")
      {
        caP->valueType          = ValueTypeBoolean;
        caP->valueValue.boolean = true;
      }
      else if (type == "False")
      {
        caP->valueType          = ValueTypeBoolean;
        caP->valueValue.boolean = false;
      }
      else if (type == "Vector")
      {
        caP->value             = iter->value.GetString();
        caP->valueType         = ValueTypeCompoundVector;
        caP->valueValue.string = (char*) caP->value.c_str();
      }
      else if (type == "Object")
      {
        caP->value             = iter->value.GetString();
        caP->valueType         = ValueTypeCompoundObject;
        caP->valueValue.string = (char*) caP->value.c_str();
      }
    }
    else  // Metadata
    {
      Metadata*   mP = new Metadata();
      std::string r  = parseMetadata(iter->value, mP);

      caP->metadataVector.push_back(mP);

      if (r != "OK")
      {
        LM_W(("Bad Input (error parsing Metadata)"));
        return "Parse Error";
      }
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute - 
*/
std::string parseContextAttribute(const Value::ConstMemberIterator& iter, ContextAttribute* caP)
{
  std::string name   = iter->name.GetString();
  std::string type   = jsonParseTypeNames[iter->value.GetType()];
  
  caP->name = name;

  if (type == "String")
  {
    caP->type               = "";
    caP->value              = iter->value.GetString();
    caP->valueType          = ValueTypeString;
    caP->valueValue.string  = (char*) iter->value.GetString();
  }
  else if (type == "Number")
  {
    caP->type               = "";
    caP->valueType          = ValueTypeNumber;
    caP->valueValue.number  = iter->value.GetDouble();
  }
  else if (type == "True")
  {
    caP->type               = "";
    caP->valueType          = ValueTypeBoolean;
    caP->valueValue.boolean = true;
  }
  else if (type == "False")
  {
    caP->type               = "";
    caP->valueType          = ValueTypeBoolean;
    caP->valueValue.boolean = false;
  }
  else if (type == "Vector")
  {
    caP->valueType  = ValueTypeCompoundVector;
    // caP->valueValue.compound = "";
  }
  else if (type == "Object")
  {
    std::string r;

    //
    // Either Compound or '{ "type": "xxx", "value": "yyy" }'
    //
    // If the Object contains "value", then it is considered an object, not a compound
    //
    if (iter->value.HasMember("value"))
    {
      r = parseContextAttributeObject(iter->value, caP);
      if (r != "OK")
      {
        LM_W(("Bad Input (json error in EntityId::ContextAttribute::Object"));
        return "Parse Error";
      }
    }
    else
    {
      caP->value              = iter->value.GetString();
      caP->valueType          = ValueTypeCompoundObject;
      caP->valueValue.string  = (char*) caP->value.c_str();
    }
  }
  else
  {
    LM_W(("Bad Input (bad type for EntityId::ContextAttribute)"));
    return "Parse Error";
  }

  return "OK";
}
