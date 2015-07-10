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



std::string parseCompound(const Value& node, ContextAttribute* caP, orion::CompoundValueNode* parent)
{
  std::string type   = jsonParseTypeNames[node.GetType()];

  if (caP->compoundValueP == NULL)
  {
    if (type == "Object")
    {
      caP->compoundValueP            = new orion::CompoundValueNode();
      caP->compoundValueP->type      = orion::CompoundValueNode::Object;
      caP->compoundValueP->container = caP->compoundValueP;
      caP->compoundValueP->name      = "TOP";
    }
    else if (type == "Vector")
    {
      caP->compoundValueP            = new orion::CompoundValueNode();
      caP->compoundValueP->type      = orion::CompoundValueNode::Vector;
      caP->compoundValueP->container = caP->compoundValueP;
      caP->compoundValueP->name      = "TOP";
    }

    for (Value::ConstMemberIterator iter = node.MemberBegin(); iter != node.MemberEnd(); ++iter)
    {
      parseCompound(iter->value, caP, caP->compoundValueP);
    }

    return "OK";
  }

  for (Value::ConstMemberIterator iter = node.MemberBegin(); iter != node.MemberEnd(); ++iter)
  {
    if (type == "String")
    {
      orion::CompoundValueNode* cvnP = new orion::CompoundValueNode();
      cvnP->name = iter->name.GetString();
      cvnP->type = orion::CompoundValueNode::String;
      
      parent->childV.push_back(cvnP);
    }
    else if (type == "Object")
    {
      orion::CompoundValueNode* cvnP = new orion::CompoundValueNode();
      cvnP->type = orion::CompoundValueNode::Object;

      for (Value::ConstMemberIterator iter = node.MemberBegin(); iter != node.MemberEnd(); ++iter)
      {
        parseCompound(iter->value, caP, cvnP);
      }
    }
    else if (type == "Vector")
    {
      orion::CompoundValueNode* cvnP = new orion::CompoundValueNode();
      cvnP->type = orion::CompoundValueNode::Vector;

      for (Value::ConstMemberIterator iter = node.MemberBegin(); iter != node.MemberEnd(); ++iter)
      {
        parseCompound(iter->value, caP, cvnP);
      }
    }
  }

  return "OK";
}



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
        caP->stringValue        = iter->value.GetString();
        caP->valueType    = ValueTypeString;
      }
      else if (type == "Number")
      {
        caP->numberValue  = iter->value.GetDouble();
        caP->valueType    = ValueTypeNumber;
      }
      else if (type == "True")
      {
        caP->boolValue    = true;
        caP->valueType    = ValueTypeBoolean;
      }
      else if (type == "False")
      {
        caP->boolValue    = false;
        caP->valueType    = ValueTypeBoolean;
      }
      else if (type == "Vector")
      {
        caP->stringValue        = iter->value.GetString();  // FIXME P9: Can't imagine this works ...
        caP->valueType    = ValueTypeCompoundVector;
      }
      else if (type == "Object")
      {
        caP->stringValue        = iter->value.GetString();  // FIXME P9: Can't imagine this works ...
        caP->valueType    = ValueTypeCompoundObject;
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
  LM_M(("KZ: In parseContextAttribute"));

  std::string name   = iter->name.GetString();
  std::string type   = jsonParseTypeNames[iter->value.GetType()];
  
  caP->name = name;

  LM_M(("KZ: name: %s", caP->name.c_str()));

  if (type == "String")
  {
    caP->type        = "";
    caP->stringValue = iter->value.GetString();
    caP->valueType   = ValueTypeString;
  }
  else if (type == "Number")
  {
    caP->type        = "";
    caP->valueType   = ValueTypeNumber;
    caP->numberValue = iter->value.GetDouble();
  }
  else if (type == "True")
  {
    caP->type        = "";
    caP->valueType   = ValueTypeBoolean;
    caP->boolValue   = true;
  }
  else if (type == "False")
  {
    caP->type        = "";
    caP->valueType   = ValueTypeBoolean;
    caP->boolValue   = false;
  }
  else if (type == "Vector")
  {
    caP->valueType  = ValueTypeCompoundVector;
    // caP->valueValue.compound = "";
  }
  else if (type == "Object")
  {
    std::string r;

    LM_M(("KZ: Object"));
    //
    // Either Compound or '{ "type": "xxx", "value": "yyy" }'
    //
    // If the Object contains "value", then it is considered an object, not a compound
    //
    if (iter->value.HasMember("value"))
    {
      LM_M(("KZ: Normal object"));
      LM_M(("KZ: Normal object (has 'value'): '%s'", iter->value.GetString()));

      r = parseContextAttributeObject(iter->value, caP);
      if (r != "OK")
      {
        LM_W(("Bad Input (json error in EntityId::ContextAttribute::Object"));
        return "Parse Error";
      }
    }
    else
    {
      LM_M(("KZ: Compound object"));
      parseCompound(iter->value, caP, NULL);
      caP->valueType    = ValueTypeCompoundObject;
    }
  }
  else
  {
    LM_W(("Bad Input (bad type for EntityId::ContextAttribute)"));
    return "Parse Error";
  }

  return "OK";
}
