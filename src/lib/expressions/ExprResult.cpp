/*
*
* Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/

// The ExprResult class is used as return value in ExprManager::evaluate(). We could return std::string
// in that function and simplify (so avoiding the ExprResult class). But in that case float rounding is
// problematic (e.g. 1.999999 instead of 2), as they don't take advantage of the ad hoc logic implemented
// in ContextAttribute rendering

#include "rapidjson/document.h"

#include "expressions/ExprResult.h"

#include "common/string.h"
#include "common/JsonHelper.h"
#include "logMsg/logMsg.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/utilsParse.h"



static void processDictItem(orion::CompoundValueNode* parentP, const rapidjson::Value::ConstMemberIterator&  iter);  // forward declaration



/* ****************************************************************************
*
* processListItem -
*
*/
static void processListItem(orion::CompoundValueNode* parentP, const rapidjson::Value::ConstValueIterator&  iter)
{
  orion::CompoundValueNode* nodeP;

  std::string type = jsonParseTypeNames[iter->GetType()];

  if (type == "String")
  {
    nodeP = new orion::CompoundValueNode("", iter->GetString(), orion::ValueTypeString);
    parentP->add(nodeP);
  }
  else if (type == "Number")
  {
    nodeP = new orion::CompoundValueNode("", iter->GetDouble(), orion::ValueTypeNumber);
    parentP->add(nodeP);
  }
  else if (type == "True")
  {
    nodeP = new orion::CompoundValueNode("", true, orion::ValueTypeBoolean);
    parentP->add(nodeP);
  }
  else if (type == "False")
  {
    nodeP = new orion::CompoundValueNode("", false, orion::ValueTypeBoolean);
    parentP->add(nodeP);
  }
  else if (type == "Null")
  {
    nodeP = new orion::CompoundValueNode("", "", orion::ValueTypeNull);
    parentP->add(nodeP);
  }
  else if (type == "Array")
  {
    nodeP = new orion::CompoundValueNode("", "", orion::ValueTypeVector);
    for (rapidjson::Value::ConstValueIterator iter2 = iter->Begin(); iter2 != iter->End(); ++iter2)
    {
      processListItem(nodeP, iter2);
    }
    parentP->add(nodeP);
  }
  else if (type == "Object")
  {
    nodeP = new orion::CompoundValueNode("", "", orion::ValueTypeObject);
    for (rapidjson::Value::ConstMemberIterator iter2 = iter->MemberBegin(); iter2 != iter->MemberEnd(); ++iter2)
    {
      processDictItem(nodeP, iter2);
    }
    parentP->add(nodeP);
  }
  else
  {
    LM_E(("Runtime Error (unknown type: %s)", type.c_str()));
  }
}



/* ****************************************************************************
*
* processDictItem -
*
*/
static void processDictItem(orion::CompoundValueNode* parentP, const rapidjson::Value::ConstMemberIterator&  iter)
{
  orion::CompoundValueNode* nodeP;

  std::string name = iter->name.GetString();
  std::string type = jsonParseTypeNames[iter->value.GetType()];

  if (type == "String")
  {
    nodeP = new orion::CompoundValueNode(name, iter->value.GetString(), orion::ValueTypeString);
    parentP->add(nodeP);
  }
  else if (type == "Number")
  {
    nodeP = new orion::CompoundValueNode(name, iter->value.GetDouble(), orion::ValueTypeNumber);
    parentP->add(nodeP);
  }
  else if (type == "True")
  {
    nodeP = new orion::CompoundValueNode(name, true, orion::ValueTypeBoolean);
    parentP->add(nodeP);
  }
  else if (type == "False")
  {
    nodeP = new orion::CompoundValueNode(name, false, orion::ValueTypeBoolean);
    parentP->add(nodeP);
  }
  else if (type == "Null")
  {
    nodeP = new orion::CompoundValueNode(name, "", orion::ValueTypeNull);
    parentP->add(nodeP);
  }
  else if (type == "Array")
  {
    nodeP = new orion::CompoundValueNode(name, "", orion::ValueTypeVector);
    for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
    {
      processListItem(nodeP, iter2);
    }
    parentP->add(nodeP);
  }
  else if (type == "Object")
  {
    nodeP = new orion::CompoundValueNode(name, "", orion::ValueTypeObject);
    for (rapidjson::Value::ConstMemberIterator iter2 = iter->value.MemberBegin(); iter2 != iter->value.MemberEnd(); ++iter2)
    {
      processDictItem(nodeP, iter2);
    }
    parentP->add(nodeP);
  }
  else
  {
    LM_E(("Runtime Error (unknown type: %s)", type.c_str()));
  }
}



/* ****************************************************************************
*
* ExprResult::ExprResult -
*
*/
ExprResult::ExprResult()
{
  compoundValueP = NULL;
}



/* ****************************************************************************
*
* ExprResult::fill -
*
*/
void ExprResult::fill(const std::string& result)
{
  // If nothing changes, the returned value would be null (failsafe)
  valueType = orion::ValueTypeNull;

  rapidjson::Document  document;

  document.Parse(result.c_str());

  if (document.HasParseError())
  {
    LM_E(("Runtime Error (parsing ExprResult: %s)", parseErrorString(document.GetParseError()).c_str()));
    return;
  }

  std::string type = jsonParseTypeNames[document.GetType()];

  if (type == "String")
  {
    stringValue  = document.GetString();
    valueType    = orion::ValueTypeString;
  }
  else if (type == "Number")
  {
    numberValue  = document.GetDouble();
    valueType    = orion::ValueTypeNumber;
  }
  else if (type == "True")
  {
    boolValue    = true;
    valueType    = orion::ValueTypeBoolean;
  }
  else if (type == "False")
  {
    boolValue    = false;
    valueType    = orion::ValueTypeBoolean;
  }
  else if (type == "Null")
  {
    valueType    = orion::ValueTypeNull;
  }
  else if (type == "Array")
  {
    valueType  = orion::ValueTypeVector;
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
    for (rapidjson::Value::ConstValueIterator iter = document.Begin(); iter != document.End(); ++iter)
    {
      processListItem(compoundValueP, iter);
    }
  }
  else if (type == "Object")
  {
    valueType  = orion::ValueTypeObject;
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
    for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
    {
      processDictItem(compoundValueP, iter);
    }
  }
  else
  {
    LM_E(("Runtime Error (unknown type: %s)", type.c_str()));
  }
}



/* ****************************************************************************
*
* ExprResult::toString -
*
* Pretty similar to ContextAttribute::toJsonValue()
*
*/
std::string ExprResult::toString(void)
{
  if (valueType == orion::ValueTypeNumber)
  {
    return double2string(numberValue);
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    return boolValue ? "true" : "false";
  }
  else if (valueType == orion::ValueTypeString)
  {
    return "\"" + stringValue + "\"";
  }
  else if ((valueType == orion::ValueTypeObject)||(valueType == orion::ValueTypeVector))
  {
    if (compoundValueP != NULL)
    {
      return compoundValueP->toJson();
    }
    else
    {
      LM_E(("Runtime Error (result is vector/object but compountValue is NULL)"));
      return "";
    }
  }
  else if (valueType == orion::ValueTypeNull)
  {
    return "null";
  }
  else
  {
    LM_E(("Runtime Error (not allowed type in ExprResult: %s)", valueTypeName(valueType)));
    return "";
  }
}



/* ****************************************************************************
*
* ExprResult::release -
*/
void ExprResult::release(void)
{
  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }
}