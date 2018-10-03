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
  boolValue   (false),
  container   (NULL),
  rootP       (NULL),
  siblingNo   (0),
  renderName  (false),
  path        ("Unset"),
  level       (0)
{
  LM_T(LmtCompoundValue, ("Created EMPTY compound node at %p", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for toplevel 'node'
*/
CompoundValueNode::CompoundValueNode(orion::ValueType _type):
  name        ("toplevel"),
  valueType   (_type),
  numberValue (0.0),
  boolValue   (false),
  container   (this),
  rootP       (this),
  siblingNo   (0),
  renderName  (false),
  path        ("/"),
  level       (0)
{
  LM_T(LmtCompoundValue, ("Created TOPLEVEL compound node (a %s) at %p", (valueType == orion::ValueTypeVector)? "Vector" : "Object", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (string)
*/
CompoundValueNode::CompoundValueNode
(
  CompoundValueNode*  _container,
  const std::string&  _path,
  const std::string&  _name,
  const std::string&  _value,
  int                 _siblingNo,
  orion::ValueType    _type,
  int                 _level
):
  name        (_name),
  valueType   (_type),
  stringValue (_value),
  numberValue (0.0),
  boolValue   (false),
  container   (_container),
  rootP       (container->rootP),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (container->level + 1)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (char*)
*/
CompoundValueNode::CompoundValueNode
(
  CompoundValueNode*  _container,
  const std::string&  _path,
  const std::string&  _name,
  const char*         _value,
  int                 _siblingNo,
  orion::ValueType    _type,
  int                 _level
):
  name        (_name),
  valueType   (_type),
  stringValue (std::string(_value)),
  numberValue (0.0),
  boolValue   (false),
  container   (_container),
  rootP       (container->rootP),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (container->level + 1)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (number)
*/
CompoundValueNode::CompoundValueNode
(
  CompoundValueNode*  _container,
  const std::string&  _path,
  const std::string&  _name,
  double              _value,
  int                 _siblingNo,
  orion::ValueType    _type,
  int                 _level
):
  name        (_name),
  valueType   (_type),
  stringValue (),
  numberValue (_value),
  boolValue   (false),
  container   (_container),
  rootP       (container->rootP),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (container->level + 1)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel (bool)
*/
CompoundValueNode::CompoundValueNode
(
  CompoundValueNode*  _container,
  const std::string&  _path,
  const std::string&  _name,
  bool                _value,
  int                 _siblingNo,
  orion::ValueType    _type,
  int                 _level
):
  name        (_name),
  valueType   (_type),
  stringValue (""),
  numberValue (0.0),
  boolValue   (_value),
  container   (_container),
  rootP       (container->rootP),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (container->level + 1)
{
  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          orion::valueTypeName(valueType),
                          this));
}



/* ****************************************************************************
*
* CompoundValueNode - destructor for all nodes except toplevel
*/
CompoundValueNode::~CompoundValueNode()
{
  LM_T(LmtCompoundValue, ("Destroying node %p: name: '%s', path '%s' at %p (with %d children)", this, name.c_str(), path.c_str(), this, childV.size()));

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
*/
std::string CompoundValueNode::finish(void)
{
  error = "OK";

  LM_T(LmtCompoundValue, ("Finishing a compound"));

  if (lmTraceIsSet(LmtCompoundValueShow))
  {
    show("");
  }

  check();  // sets 'error' for toplevel node

  return error;
}



/* ****************************************************************************
*
* add -
*/
CompoundValueNode* CompoundValueNode::add(CompoundValueNode* node)
{
  node->container = this;
  node->level     = level + 1;
  node->siblingNo = childV.size();
  node->rootP     = rootP;

  if (node->valueType == orion::ValueTypeString)
    LM_T(LmtCompoundValueAdd, ("Adding String '%s', with value '%s' under '%s' (%s)",
                               node->name.c_str(),
                               node->stringValue.c_str(),
                               node->container->path.c_str(),
                               node->container->name.c_str()));
  else
    LM_T(LmtCompoundValueAdd, ("Adding %s '%s' under '%s' (%s)", orion::valueTypeName(node->valueType), node->name.c_str(),
                               node->container->path.c_str(),
                               node->container->name.c_str()));

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
  std::string newPath = path;

  if (newPath == "/")
  {
    newPath += _name;
  }
  else
  {
    newPath += "/" + _name;
  }

  CompoundValueNode* node = new CompoundValueNode(this, newPath, _name, _value, childV.size(), _type, level + 1);

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
  std::string newPath = path;

  if (newPath == "/")
  {
    newPath += _name;
  }
  else
  {
    newPath += "/" + _name;
  }

  CompoundValueNode* node = new CompoundValueNode(this, newPath, _name, _value, childV.size(), _type, level + 1);

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
  std::string newPath = path;

  if (newPath == "/")
  {
    newPath += _name;
  }
  else
  {
    newPath += "/" + _name;
  }

  CompoundValueNode* node = new CompoundValueNode(this, newPath, _name, _value, childV.size(), _type, level + 1);

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
  std::string newPath = path;

  if (newPath == "/")
  {
    newPath += _name;
  }
  else
  {
    newPath += "/" + _name;
  }

  CompoundValueNode* node = new CompoundValueNode(this, newPath, _name, _value, childV.size(), _type, level + 1);

  return add(node);
}



/* ****************************************************************************
*
* shortShow -
*/
void CompoundValueNode::shortShow(const std::string& indent)
{
  if ((rootP == this) && (valueType == orion::ValueTypeVector))
  {
    LM_T(LmtCompoundValue,      ("%s%s (toplevel vector)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (rootP == this)
  {
    LM_T(LmtCompoundValue,      ("%s%s (toplevel object)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeVector)
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

  LM_T(LmtCompoundValueShow, ("%scontainer: %s",
                              indent.c_str(),
                              container->name.c_str()));
  LM_T(LmtCompoundValueShow, ("%slevel:     %d",
                              indent.c_str(),
                              level));
  LM_T(LmtCompoundValueShow, ("%ssibling:   %d",
                              indent.c_str(),
                              siblingNo));
  LM_T(LmtCompoundValueShow, ("%stype:      %s",
                              indent.c_str(),
                              orion::valueTypeName(valueType)));
  LM_T(LmtCompoundValueShow, ("%spath:      %s",
                              indent.c_str(),
                              path.c_str()));
  LM_T(LmtCompoundValueShow, ("%srootP:     %s",
                              indent.c_str(),
                              rootP->name.c_str()));

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
* Encountered errors are saved in the 'error' field of the root of the tree (rootP->error).
*/
std::string CompoundValueNode::check(void)
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
        rootP->error =
          std::string("bad tag-name of vector item: /") + childV[ix]->name + "/, should be /" + childV[0]->name + "/";

        alarmMgr.badInput(clientIp, rootP->error);
        return rootP->error;
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
          rootP->error = std::string("duplicated tag-name: /") + childV[ix]->name + "/ in path: " + path;
          alarmMgr.badInput(clientIp, rootP->error);

          return rootP->error;
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
    res = childV[ix]->check();
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
std::string CompoundValueNode::toJson(bool toplevel)
{
  std::string      out;
  JsonVectorHelper jh;

  switch (valueType)
  {
  case orion::ValueTypeString:
    out = '"';
    out += toJsonString(stringValue);
    out += '"';
    break;

  case orion::ValueTypeNumber:
    out = double2string(numberValue);
    break;

  case orion::ValueTypeBoolean:
    out = boolValue? "true" : "false";
    break;

  case orion::ValueTypeNull:
    out = "null";
    break;

  case orion::ValueTypeVector:
    for (unsigned int ix = 0; ix < childV.size(); ix++)
    {
      jh.addRaw(childV[ix]->toJson(false));
    }
    out = jh.str();
    break;

  case orion::ValueTypeObject:
    // In thic case we cannot use JsonHelper to build the object, as we don't have a
    // key-value sequence to invoke addXX() method
    if (childV.size() == 0)
    {
      out = "{}";
    }
    else
    {
      out = "{";
      out += childV[0]->toJson(false);
      for (unsigned int ix = 1; ix < childV.size(); ix++)
      {
        out += ",";
        out += childV[ix]->toJson(false);
      }
      out += "}";
    }
    // Early return in this case, to avoid getting parentIsObject as in the
    // case of root element we don't use key. Only the first call to
    // toJson() uses toplevel == true
    if (toplevel)
    {
      return out;
    }
    break;

  case orion::ValueTypeNotGiven:
    LM_E(("Runtime Error (value type not given (%s))", name.c_str()));
    return "";

  default:
    LM_E(("Runtime Error (value type unknown (%s))", name.c_str()));
    return "";
  }

  bool parentIsObject = (container->valueType == orion::ValueTypeObject);

  if (parentIsObject)
  {
    std::string preOut = "\"";
    preOut += toJsonString(name);
    preOut += "\":";
    return preOut + out;
  }
  else
  {
    return out;
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

  if (rootP == this)
  {
    me = new CompoundValueNode(valueType);
  }
  else
  {
    switch (valueType)
    {
    case orion::ValueTypeString:
    case orion::ValueTypeObject:
    case orion::ValueTypeVector:
      me = new CompoundValueNode(container, path, name, stringValue, siblingNo, valueType, level);
      break;

    case orion::ValueTypeNumber:
      me = new CompoundValueNode(container, path, name, numberValue, siblingNo, valueType, level);
      break;

    case orion::ValueTypeBoolean:
      me = new CompoundValueNode(container, path, name, boolValue, siblingNo, valueType, level);
      break;

    case orion::ValueTypeNull:
      me = new CompoundValueNode(container, path, name, stringValue, siblingNo, valueType, level);
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



/* ****************************************************************************
*
* cpath -
*/
const char* CompoundValueNode::cpath(void)
{
  return path.c_str();
}
}
