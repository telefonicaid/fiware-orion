/*
*
* Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/

#include <string>

#include "jsonParseV2/parseCompoundCommon.h"

#include "common/limits.h"
#include "jsonParseV2/jsonParseTypeNames.h"

/* ****************************************************************************
*
* stringToCompoundType -
*
*/
orion::ValueType stringToCompoundType(std::string nodeType)
{
  if      (nodeType == "String")  return orion::ValueTypeString;
  else if (nodeType == "Number")  return orion::ValueTypeNumber;
  else if (nodeType == "True")    return orion::ValueTypeBoolean;
  else if (nodeType == "False")   return orion::ValueTypeBoolean;
  else if (nodeType == "Object")  return orion::ValueTypeObject;
  else if (nodeType == "Array")   return orion::ValueTypeVector;
  else if (nodeType == "Null")    return orion::ValueTypeNull;

  return orion::ValueTypeString;
}



/* ****************************************************************************
*
* parseCompoundValue -
*/
std::string parseCompoundValue
(
  const rapidjson::Value::ConstValueIterator&   node,
  orion::CompoundValueNode*                     parent,
  int                                           deep
)
{
  if (deep > MAX_JSON_NESTING)
  {
    // It would be better to do this at rapidjson parsing stage, but I'm not sure if it can be done.
    // There is a question about it in SOF https://stackoverflow.com/questions/60735627/limit-json-nesting-level-at-parsing-stage-in-rapidjson
    // Depending of the answer, this check could be removed (along with the deep parameter
    // in this an another functions and the returns check for "max deep reached"), i.e.
    // revert the changes in commit ce3cf0766

    return "max deep reached";
  }

  if (node->IsObject())
  {
    for (rapidjson::Value::ConstMemberIterator iter = node->MemberBegin(); iter != node->MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->valueType  = stringToCompoundType(nodeType);

      cvnP->name       = iter->name.GetString();

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
        cvnP->valueType = orion::ValueTypeNull;
      }
      else if (nodeType == "Object")
      {
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, deep + 1);
        if (r != "OK")
        {
          return r;
        }
      }
    }
  }
  else if (node->IsArray())
  {
    for (rapidjson::Value::ConstValueIterator iter = node->Begin(); iter != node->End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();

      cvnP->valueType  = stringToCompoundType(nodeType);

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
        cvnP->boolValue = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNull;
      }
      else if (nodeType == "Object")
      {
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, deep + 1);
        if (r != "OK")
        {
          return r;
        }
      }
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseCompoundValue -
*/
std::string parseCompoundValue
(
  const rapidjson::Value::ConstMemberIterator&  node,
  orion::CompoundValueNode*                     parent,
  int                                           deep
)
{
  if (deep > MAX_JSON_NESTING)
  {
    // It would be better to do this at rapidjson parsing stage, but I'm not sure if it can be done.
    // There is a question about it in SOF https://stackoverflow.com/questions/60735627/limit-json-nesting-level-at-parsing-stage-in-rapidjson
    // Depending of the answer, this check could be removed (along with the deep parameter
    // in this an another functions and the returns check for "max deep reached"), i.e.
    // revert the changes in commit ce3cf0766

    return "max deep reached";
  }

  std::string type   = jsonParseTypeNames[node->value.GetType()];

  //
  // Children of the node
  //
  if (type == "Array")
  {
    for (rapidjson::Value::ConstValueIterator iter = node->value.Begin(); iter != node->value.End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();

      cvnP->valueType  = stringToCompoundType(nodeType);

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
        cvnP->boolValue = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Object")
      {
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, deep + 1);
        if (r != "OK")
        {
          return r;
        }
      }
    }
  }
  else if (type == "Object")
  {
    rapidjson::Value::ConstMemberIterator  iter;

    for (iter = node->value.MemberBegin(); iter != node->value.MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->name       = iter->name.GetString();
      cvnP->valueType  = stringToCompoundType(nodeType);

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
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Recursive call if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, deep + 1);
        if (r != "OK")
        {
          return r;
        }
      }
    }
  }

  return "OK";
}

