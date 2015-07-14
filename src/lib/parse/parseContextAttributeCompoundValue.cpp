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
#include "ngsi/ContextAttribute.h"
#include "parse/jsonParseTypeNames.h"
#include "parse/CompoundValueNode.h"

#include "parse/parseContextAttributeCompoundValue.h"



/* ****************************************************************************
*
* stringToCompoundType - 
*/
static orion::CompoundValueNode::Type stringToCompoundType(std::string nodeType)
{
  if      (nodeType == "String")  return orion::CompoundValueNode::String;
  else if (nodeType == "Number")  return orion::CompoundValueNode::Number;
  else if (nodeType == "True")    return orion::CompoundValueNode::Bool;
  else if (nodeType == "False")   return orion::CompoundValueNode::Bool;
  else if (nodeType == "Object")  return orion::CompoundValueNode::Object;
  else if (nodeType == "Array")   return orion::CompoundValueNode::Vector;

  return orion::CompoundValueNode::String;
}



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
  else if (cvnP->type == orion::CompoundValueNode::Number)
  {
    sprintf(buffer, "%f", cvnP->numberValue);
    return buffer;
  }

  return (char*) "unknown type";
}



/* ****************************************************************************
*
* parseContextAttributeCompoundValue - 
*/
std::string parseContextAttributeCompoundValue
(
  const Value::ConstMemberIterator&  node,
  ContextAttribute*                  caP,
  orion::CompoundValueNode*          parent
)
{
  std::string type   = jsonParseTypeNames[node->value.GetType()];
  std::string name   = node->name.GetString();

  LM_M(("Got: %s (%s)", name.c_str(), type.c_str()));

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
  }


  //
  // Children of the node
  //
  if (type == "Array")
  {
    int counter  = 0;

    LM_M(("1"));
    // Value::ConstMemberIterator memberIterator = node->value.MemberBegin();
    LM_M(("2"));
    for (Value::ConstValueIterator iter = node->value.Begin(); iter != node->value.End(); ++iter) // , ++memberIterator)
    {
      LM_M(("3"));
      char                       buffer[256];
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();
      char                       itemNo[4];

      LM_M(("3"));
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
      LM_M(("KZ: pushed Array-member %d: %s: %s (%s)", counter, nodeType.c_str(), cvnP->path.c_str(), stringValue(cvnP, buffer)));

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        LM_M(("Object/Array inside Array - not implemnented!"));
        // Value::ConstMemberIterator node2 = node->FindMember(iter);
        // LM_M(("Calling parseContextAttributeCompoundValue"));
        // parseContextAttributeCompoundValue(node2, caP, cvnP);
      }

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
        LM_M(("Recursive call as Object or Array"));
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }

      ++counter;
    }
  }

  return "OK";
}
