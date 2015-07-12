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
* stringValue - 
*/
static char* stringValue(orion::CompoundValueNode* cvnP, char* buffer)
{
  if (cvnP->type == orion::CompoundValueNode::String)
  {
    return (char*) cvnP->value.c_str();
  }
  else if (cvnP->type == orion::CompoundValueNode::Number)
  {
    sprintf(buffer, "%f", cvnP->numberValue);
    return buffer;
  }
  else if (cvnP->type == orion::CompoundValueNode::Bool)
  {
    return (char*) "Boolean";
  }
  else if (cvnP->type == orion::CompoundValueNode::Object)
  {
    return (char*) "Object";
  }
  else if (cvnP->type == orion::CompoundValueNode::Vector)
  {
    return (char*) "Array";
  }

  return (char*) "unknown type";
}



/* ****************************************************************************
*
* stringToCompoundType - 
*/
static orion::CompoundValueNode::Type stringToCompoundType(std::string nodeType)
{
  if (nodeType == "String")
  {
    return orion::CompoundValueNode::String;
  }
  else if (nodeType == "Number")
  {
    return orion::CompoundValueNode::Number;
  }
  else if ((nodeType == "True") || (nodeType == "False"))
  {
    return orion::CompoundValueNode::Bool;
  }
  else if (nodeType == "Object")
  {
    return orion::CompoundValueNode::Object;
  }
  else if (nodeType == "Array")
  {
    return orion::CompoundValueNode::Vector;
  }

  return orion::CompoundValueNode::String;
}



/* ****************************************************************************
*
* parseCompound - 
*/
std::string parseCompound(const Value::ConstMemberIterator& node, ContextAttribute* caP, orion::CompoundValueNode* parent)
{
  std::string type   = jsonParseTypeNames[node->value.GetType()];
  std::string name   = node->name.GetString();

  // Create this node
  if (caP->compoundValueP == NULL)
  {
    LM_M(("KZ-Comp: Got a TOPLEVEL %s", type.c_str()));
    caP->compoundValueP            = new orion::CompoundValueNode();
    caP->compoundValueP->name      = "TOP";
    caP->compoundValueP->container = caP->compoundValueP;
    caP->compoundValueP->type      = stringToCompoundType(type);
    caP->compoundValueP->path      = "/";
    caP->compoundValueP->rootP     = caP->compoundValueP;
    caP->compoundValueP->level     = 0;
    caP->compoundValueP->siblingNo = 0;

    parent = caP->compoundValueP;

//    if (caP->type == "")
//    {
//      caP->type = (type == "Object")? "Compound Object" : "Compound Vector";
//    }
  }


  //
  // Children of the node
  //
  if (type == "Array")
  {
    int counter  = 0;

    for (Value::ConstValueIterator iter = node->value.Begin(); iter != node->value.End(); ++iter)
    {
      char                       buffer[256];
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();
      char                       itemNo[4];

      snprintf(itemNo, sizeof(itemNo), "%03d", counter);

      cvnP->type       = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + "[" + itemNo + "]";


      if (nodeType == "String")
      {
        cvnP->value       = iter->GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }

      parent->childV.push_back(cvnP);
      LM_M(("KZ: pushed Array-member %d: %s: %s (%s)", counter, nodeType.c_str(), cvnP->path.c_str(), stringValue(cvnP, buffer)));
      ++counter;
    }
  }
  else if (type == "Object")
  {
    int counter  = 0;

    for (Value::ConstMemberIterator iter = node->value.MemberBegin(); iter != node->value.MemberEnd(); ++iter)
    {
      char                       buffer[256];
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->name       = iter->name.GetString();
      cvnP->type       = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + cvnP->name;

      if (nodeType == "String")
      {
        cvnP->value       = iter->value.GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->value.GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Object")
      {
        cvnP->path += "/";
        cvnP->type = orion::CompoundValueNode::Object;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->type = orion::CompoundValueNode::Vector;
      }

      parent->childV.push_back(cvnP);
      LM_M(("KZ: pushed %s: %s (%s)", nodeType.c_str(), cvnP->path.c_str(), stringValue(cvnP, buffer)));
        
      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseCompound(iter, caP, cvnP);
      }

      ++counter;
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
      else if (type == "Array")
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
  else if (type == "Array")
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
      parseCompound(iter, caP, NULL);
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
