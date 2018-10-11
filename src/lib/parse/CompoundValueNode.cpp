/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/string.h"
#include "common/tag.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"
#include "parse/forbiddenChars.h"

#include "orionTypes/OrionValueType.h"
#include "parse/CompoundValueNode.h"



namespace orion
{
/* ****************************************************************************
*
* CompoundValueNode - constructor for toplevel 'node'
*/
CompoundValueNode::CompoundValueNode():
  name        ("Unset"),
  valueType   (orion::ValueTypeNotGiven),
  numberValue (0.0),
  boolValue   (false)
{
  LM_T(LmtCompoundValue, ("Created EMPTY compound node at %p", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for toplevel 'node'
*/
CompoundValueNode::CompoundValueNode(orion::ValueType _type):
  name        (""),
  valueType   (_type),
  numberValue (0.0),
  boolValue   (false)
{
  LM_T(LmtCompoundValue, ("Created TOPLEVEL compound node (a %s) at %p", (valueType == orion::ValueTypeVector)? "Vector" : "Object", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (string)
*/
CompoundValueNode::CompoundValueNode
(
  const std::string&  _name,
  const std::string&  _value,
  orion::ValueType    _type
):
  name        (_name),
  valueType   (_type),
  stringValue (_value),
  numberValue (0.0),
  boolValue   (false)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s', type %s at %p",
                          name.c_str(),
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (char*)
*/
CompoundValueNode::CompoundValueNode
(
  const std::string&  _name,
  const char*         _value,
  orion::ValueType    _type
):
  name        (_name),
  valueType   (_type),
  stringValue (std::string(_value)),
  numberValue (0.0),
  boolValue   (false)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s', type %s at %p",
                          name.c_str(),
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (number)
*/
CompoundValueNode::CompoundValueNode
(
  const std::string&  _name,
  double              _value,
  orion::ValueType    _type
):
  name        (_name),
  valueType   (_type),
  stringValue (),
  numberValue (_value),
  boolValue   (false)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s', type %s at %p",
                          name.c_str(),
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (bool)
*/
CompoundValueNode::CompoundValueNode
(
  const std::string&  _name,
  bool                _value,
  orion::ValueType    _type
):
  name        (_name),
  valueType   (_type),
  stringValue (""),
  numberValue (0.0),
  boolValue   (_value)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s', type %s at %p",
                          name.c_str(),
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - destructor for all nodes except toplevel
*/
CompoundValueNode::~CompoundValueNode()
{
  LM_T(LmtCompoundValue, ("Destroying node %p: name: '%s' at %p (with %d children)", this, name.c_str(), this, childV.size()));

  for (uint64_t ix = 0; ix < childV.size(); ++ix)
  {
    if (childV[ix] != NULL)
    {
      LM_T(LmtCompoundValue, ("Deleting child %d, at %p", ix, childV[ix]));
      delete childV[ix];
      childV[ix] = NULL;
    }
  }

  while (childV.size() != 0)
  {
    childV.erase(childV.begin());
  }

  childV.clear();
}



/* ****************************************************************************
*
* finish -
*
*/
std::string CompoundValueNode::finish(void)
{
  std::string error = "OK";

  LM_T(LmtCompoundValue, ("Finishing a compound"));

  if (lmTraceIsSet(LmtCompoundValueShow))
  {
    show("");
  }

  error = check("");  // sets 'error' for toplevel node

  return error;
}



/* ****************************************************************************
*
* add -
*/
CompoundValueNode* CompoundValueNode::add(CompoundValueNode* node)
{
  if (node->valueType == orion::ValueTypeString)
  {
    LM_T(LmtCompoundValueAdd, ("Adding String '%s', with value '%s'",
                               node->name.c_str(),
                               node->stringValue.c_str()));
  }
  else
  {
    LM_T(LmtCompoundValueAdd, ("Adding %s '%s')", orion::valueTypeName(node->valueType), node->name.c_str()));
  }

  childV.push_back(node);
  return node;
}



/* ****************************************************************************
*
* add - (string)
*/
CompoundValueNode* CompoundValueNode::add
(
  const orion::ValueType  _type,
  const std::string&      _name,
  const std::string&      _value
)
{
  CompoundValueNode* node = new CompoundValueNode(_name, _value, _type);

  return add(node);
}



/* ****************************************************************************
*
* add - (char*)
*/
CompoundValueNode* CompoundValueNode::add
(
  const orion::ValueType  _type,
  const std::string&      _name,
  const char*             _value
)
{
  CompoundValueNode* node = new CompoundValueNode(_name, _value, _type);

  return add(node);
}



/* ****************************************************************************
*
* add - (double)
*/
CompoundValueNode* CompoundValueNode::add
(
  const orion::ValueType  _type,
  const std::string&      _name,
  double                  _value
)
{
  CompoundValueNode* node = new CompoundValueNode(_name, _value, _type);

  return add(node);
}



/* ****************************************************************************
*
* add - (bool)
*/
CompoundValueNode* CompoundValueNode::add
(
  const orion::ValueType  _type,
  const std::string&      _name,
  bool                    _value
)
{
  CompoundValueNode* node = new CompoundValueNode(_name, _value, _type);

  return add(node);
}



/* ****************************************************************************
*
* shortShow -
*/
void CompoundValueNode::shortShow(const std::string& indent)
{
  if (valueType == orion::ValueTypeVector)
  {
    LM_T(LmtCompoundValue,      ("%s%s (vector)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeObject)
  {
    LM_T(LmtCompoundValue,      ("%s%s (object)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeString)
  {
    LM_T(LmtCompoundValue,      ("%s%s (%s)",
                                 indent.c_str(),
                                 name.c_str(),
                                 stringValue.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    LM_T(LmtCompoundValue,      ("%s%s (%s)",
                                 indent.c_str(),
                                 name.c_str(),
                                 (boolValue == true)? "true" : "false"));
    return;
  }
  else if (valueType == orion::ValueTypeNull)
  {
    LM_T(LmtCompoundValue,      ("%s%s (null)",
                                 indent.c_str(),
                                 name.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_T(LmtCompoundValue,      ("%s%s (not given)",
                                 indent.c_str(),
                                 name.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    LM_T(LmtCompoundValue,      ("%s%s (%f)",
                                 indent.c_str(),
                                 name.c_str(),
                                 numberValue));
    return;
  }

  for (uint64_t ix = 0; ix < childV.size(); ++ix)
  {
    childV[ix]->shortShow(indent + "  ");
  }
}



/* ****************************************************************************
*
* show -
*/
void CompoundValueNode::show(const std::string& indent)
{
  if (name != "")
  {
    LM_T(LmtCompoundValueShow, ("%sname:      %s",
                                indent.c_str(),
                                name.c_str()));
  }

  LM_T(LmtCompoundValueShow, ("%stype:      %s",
                              indent.c_str(),
                              orion::valueTypeName(valueType)));

  if (valueType == orion::ValueTypeString)
  {
    LM_T(LmtCompoundValueShow, ("%sString Value:     %s",
                                indent.c_str(),
                                stringValue.c_str()));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    LM_T(LmtCompoundValueShow, ("%sBool Value:     %s",
                                indent.c_str(),
                                (boolValue == false)? "false" : "true"));
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    LM_T(LmtCompoundValueShow, ("%sNumber Value:     %f",
                                indent.c_str(),
                                numberValue));
  }
  else if (valueType == orion::ValueTypeNull)
  {
    LM_T(LmtCompoundValueShow, ("%sNull",
                                indent.c_str()));
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_T(LmtCompoundValueShow, ("%sNotGiven",
                                indent.c_str()));
  }
  else if (childV.size() != 0)
  {
    std::string childrenString;

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      childrenString += childV[ix]->name;
      if (ix != childV.size() - 1)
      {
        childrenString += ", ";
      }
    }

    LM_T(LmtCompoundValueShow, ("%s%lu children (%s)",
                                indent.c_str(),
                                childV.size(),
                                childrenString.c_str()));

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      childV[ix]->show(indent + "  ");
    }
  }

  LM_T(LmtCompoundValueShow, (""));
}



/* ****************************************************************************
*
* check -
*
* A vector must have all its children with the same name.
* An object cannot have two children with the same name.
*
*/
std::string CompoundValueNode::check(const std::string& path)
{
  if (valueType == orion::ValueTypeVector)
  {
    if (childV.size() == 0)
    {
      return "OK";
    }

    for (uint64_t ix = 1; ix < childV.size(); ++ix)
    {
      if (childV[ix]->name != childV[0]->name)
      {
        std::string error = "bad tag-name of vector item: /" + childV[ix]->name + "/, should be /" + childV[0]->name + "/";
        alarmMgr.badInput(clientIp, error);
        return error;
      }
    }
  }
  else if (valueType == orion::ValueTypeObject)
  {
    if (childV.size() == 0)
    {
      return "OK";
    }

    for (uint64_t ix = 0; ix < childV.size() - 1; ++ix)
    {
      for (uint64_t ix2 = ix + 1; ix2 < childV.size(); ++ix2)
      {
        if (childV[ix]->name == childV[ix2]->name)
        {
          std::string fullPath = (path == "" ? "/" : path + name);
          std::string error = "duplicated tag-name: /" + childV[ix]->name + "/ in path: " + fullPath;
          alarmMgr.badInput(clientIp, error);

          return error;
        }
      }
    }
  }
  else
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of an attribute");
      return "Invalid characters in attribute value";
    }
  }

  // 'recursively' call the check method for all children
  std::string res;

  for (uint64_t ix = 0; ix < childV.size(); ++ix)
  {
    res = childV[ix]->check(path + name + "/");
    if (res !="OK")
    {
      return res;
    }
  }
  return "OK";
}



/* ****************************************************************************
*
* CompoundValueNode:toJson
*
*/
std::string CompoundValueNode::toJson(void)
{
  std::string      out;
  JsonVectorHelper jvh;
  JsonObjectHelper joh;

  switch (valueType)
  {
  case orion::ValueTypeString:
    out = '"';
    out += toJsonString(stringValue);
    out += '"';
    return out;

  case orion::ValueTypeNumber:
    return double2string(numberValue);

  case orion::ValueTypeBoolean:
    return (boolValue? "true" : "false");

  case orion::ValueTypeNull:
    return "null";

  case orion::ValueTypeVector:
    for (unsigned int ix = 0; ix < childV.size(); ix++)
    {
      jvh.addRaw(childV[ix]->toJson());
    }
    return jvh.str();

  case orion::ValueTypeObject:
    for (unsigned int ix = 0; ix < childV.size(); ix++)
    {
      joh.addRaw(childV[ix]->name, childV[ix]->toJson());
    }
    return joh.str();

  case orion::ValueTypeNotGiven:
    LM_E(("Runtime Error (value type not given (%s))", name.c_str()));
    return "";

  default:
    LM_E(("Runtime Error (value type unknown (%s))", name.c_str()));
    return "";
  }
}



/* ****************************************************************************
*
* clone -
*/
CompoundValueNode* CompoundValueNode::clone(void)
{
  CompoundValueNode* me;

  LM_T(LmtCompoundValue, ("cloning '%s'", name.c_str()));

  switch (valueType)
  {
  case orion::ValueTypeString:
  case orion::ValueTypeObject:
  case orion::ValueTypeVector:
    me = new CompoundValueNode(name, stringValue, valueType);
    break;

  case orion::ValueTypeNumber:
    me = new CompoundValueNode(name, numberValue, valueType);
    break;

  case orion::ValueTypeBoolean:
    me = new CompoundValueNode(name, boolValue, valueType);
    break;

  case orion::ValueTypeNull:
    me = new CompoundValueNode(name, stringValue, valueType);
    me->valueType = orion::ValueTypeNull;
    break;

  case orion::ValueTypeNotGiven:
    me = NULL;
    LM_E(("Runtime Error (value not given in compound node value)"));
    break;

  default:
    me = NULL;
    LM_E(("Runtime Error (unknown compound node value type: %d)", valueType));
  }

  for (unsigned int ix = 0; ix < childV.size(); ++ix)
  {
    LM_T(LmtCompoundValue, ("Adding child %d for '%s'", ix, name.c_str()));
    me->add(childV[ix]->clone());
  }

  return me;
}



/* ****************************************************************************
*
* isVector -
*/
bool CompoundValueNode::isVector(void)
{
  return (valueType == orion::ValueTypeVector);
}



/* ****************************************************************************
*
* isObject -
*/
bool CompoundValueNode::isObject(void)
{
  return (valueType == orion::ValueTypeObject);
}



/* ****************************************************************************
*
* isString -
*/
bool CompoundValueNode::isString(void)
{
  return (valueType == orion::ValueTypeString);
}



/* ****************************************************************************
*
* cname -
*/
const char* CompoundValueNode::cname(void)
{
  return name.c_str();
}
}
