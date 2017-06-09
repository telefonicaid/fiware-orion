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
#include "parse/CompoundValueNode.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseContextAttributeCompoundValue.h"



/* ****************************************************************************
*
* stringToCompoundType -
*/
static orion::ValueType stringToCompoundType(std::string nodeType)
{
  if      (nodeType == "String")  return orion::ValueTypeString;
  else if (nodeType == "Number")  return orion::ValueTypeNumber;
  else if (nodeType == "True")    return orion::ValueTypeBoolean;
  else if (nodeType == "False")   return orion::ValueTypeBoolean;
  else if (nodeType == "Object")  return orion::ValueTypeObject;
  else if (nodeType == "Array")   return orion::ValueTypeVector;
  else if (nodeType == "Null")    return orion::ValueTypeNone;

  return orion::ValueTypeString;
}



/* ****************************************************************************
*
* parseContextAttributeCompoundValue -
*/
std::string parseContextAttributeCompoundValue
(
  const rapidjson::Value::ConstValueIterator&   node,
  ContextAttribute*                             caP,
  orion::CompoundValueNode*                     parent
)
{
  if (node->IsObject())
  {
    int counter  = 0;

    for (rapidjson::Value::ConstMemberIterator iter = node->MemberBegin(); iter != node->MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->valueType  = stringToCompoundType(nodeType);

      cvnP->name       = iter->name.GetString();
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + cvnP->name;

      if (nodeType == "String")
      {
        cvnP->stringValue       = iter->value.GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->value.GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNone;
      }
      else if (nodeType == "Object")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }

      ++counter;
    }
  }
  else if (node->IsArray())
  {
    int counter  = 0;

    for (rapidjson::Value::ConstValueIterator iter = node->Begin(); iter != node->End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();
      char                       itemNo[4];

      snprintf(itemNo, sizeof(itemNo), "%03d", counter);

      cvnP->valueType  = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + "[" + itemNo + "]";


      if (nodeType == "String")
      {
        cvnP->stringValue       = iter->GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNone;
      }
      else if (nodeType == "Object")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }

      ++counter;
    }
  }


  return "OK";
}



/* ****************************************************************************
*
* parseContextAttributeCompoundValue -
*/
std::string parseContextAttributeCompoundValue
(
  const rapidjson::Value::ConstMemberIterator&  node,
  ContextAttribute*                             caP,
  orion::CompoundValueNode*                     parent
)
{
  std::string type = jsonParseTypeNames[node->value.GetType()];

  if (caP->compoundValueP == NULL)
  {
    caP->compoundValueP            = new orion::CompoundValueNode();
    caP->compoundValueP->name      = "TOP";
    caP->compoundValueP->container = caP->compoundValueP;
    caP->compoundValueP->valueType = stringToCompoundType(type);
    caP->compoundValueP->path      = "/";
    caP->compoundValueP->rootP     = caP->compoundValueP;
    caP->compoundValueP->level     = 0;
    caP->compoundValueP->siblingNo = 0;

    parent = caP->compoundValueP;

    if (!caP->typeGiven)
    {
      caP->type = (type == "Object")? defaultType(orion::ValueTypeObject) : defaultType(orion::ValueTypeVector);
    }
  }


  //
  // Children of the node
  //
  if (type == "Array")
  {
    int counter  = 0;

    for (rapidjson::Value::ConstValueIterator iter = node->value.Begin(); iter != node->value.End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();
      char                       itemNo[4];

      snprintf(itemNo, sizeof(itemNo), "%03d", counter);

      cvnP->valueType  = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + "[" + itemNo + "]";


      if (nodeType == "String")
      {
        cvnP->stringValue       = iter->GetString();
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
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }

      ++counter;
    }
  }
  else if (type == "Object")
  {
    int counter  = 0;

    rapidjson::Value::ConstMemberIterator iter;
    for (iter = node->value.MemberBegin(); iter != node->value.MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->name       = iter->name.GetString();
      cvnP->valueType  = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + cvnP->name;

      if (nodeType == "String")
      {
        cvnP->stringValue       = iter->value.GetString();
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
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }

      ++counter;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttributeCompoundValueStandAlone -
*/
std::string parseContextAttributeCompoundValueStandAlone
(
  rapidjson::Document&  document,
  ContextAttribute*     caP,
  orion::ValueType      valueType
)
{
  caP->compoundValueP            = new orion::CompoundValueNode();
  caP->compoundValueP->name      = "TOP";
  caP->compoundValueP->container = caP->compoundValueP;
  caP->compoundValueP->valueType = caP->valueType;  // Convert to other type?
  caP->compoundValueP->path      = "/";
  caP->compoundValueP->rootP     = caP->compoundValueP;
  caP->compoundValueP->level     = 0;
  caP->compoundValueP->siblingNo = 0;

  orion::CompoundValueNode*   parent  = caP->compoundValueP;


  //
  // Children of the node
  //
  if (caP->valueType == orion::ValueTypeVector)
  {
    int counter  = 0;

    for (rapidjson::Value::ConstValueIterator iter = document.Begin(); iter != document.End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();
      char                       itemNo[4];

      snprintf(itemNo, sizeof(itemNo), "%03d", counter);

      cvnP->valueType  = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + "[" + itemNo + "]";


      if (nodeType == "String")
      {
        cvnP->stringValue = iter->GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNone;
      }
      else if (nodeType == "Object")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }
      else if (!caP->typeGiven)
      {
        caP->type = defaultType(caP->valueType);
      }

      ++counter;
    }
  }
  else if (caP->valueType == orion::ValueTypeObject)
  {
    int counter  = 0;

    for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->name       = iter->name.GetString();
      cvnP->valueType  = stringToCompoundType(nodeType);
      cvnP->container  = parent;
      cvnP->rootP      = parent->rootP;
      cvnP->level      = parent->level + 1;
      cvnP->siblingNo  = counter;
      cvnP->path       = parent->path + cvnP->name;

      if (nodeType == "String")
      {
        cvnP->stringValue = iter->value.GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->value.GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNone;
      }
      else if (nodeType == "Object")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->path += "/";
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        parseContextAttributeCompoundValue(iter, caP, cvnP);
      }
      else if (!caP->typeGiven)
      {
        caP->type = defaultType(caP->valueType);
      }

      ++counter;
    }
  }

  return "OK";
}
