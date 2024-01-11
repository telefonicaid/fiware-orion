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

#include "orionld/types/ApiVersion.h"

#include "common/globals.h"
#include "common/string.h"
#include "common/tag.h"
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
  rootP       (NULL),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (0)
{
  if (container != NULL)
  {
    rootP = container->rootP;
    level = container->level + 1;
  }
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
  rootP       (NULL),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (0)
{
  if (container != NULL)
  {
    rootP = container->rootP;
    level = container->level + 1;
  }
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
  rootP       (NULL),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (0)
{
  if (_container != NULL)
  {
    rootP = container->rootP;
    level = container->level + 1;
  }
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
  rootP       (NULL),
  siblingNo   (_siblingNo),
  renderName  (false),
  path        (_path),
  level       (0)
{
  if (_container != NULL)
  {
    rootP = container->rootP;
    level = container->level + 1;
  }
}



/* ****************************************************************************
*
* CompoundValueNode - destructor for all nodes except toplevel
*/
CompoundValueNode::~CompoundValueNode()
{
  for (uint64_t ix = 0; ix < childV.size(); ++ix)
  {
    if (childV[ix] != NULL)
    {
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

  if (lmTraceIsSet(LmtLegacy))
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
    LM_T(LmtLegacy,      ("%s%s (toplevel vector)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (rootP == this)
  {
    LM_T(LmtLegacy,      ("%s%s (toplevel object)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeVector)
  {
    LM_T(LmtLegacy,      ("%s%s (vector)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeObject)
  {
    LM_T(LmtLegacy,      ("%s%s (object)",
                                 indent.c_str(),
                                 name.c_str()));
  }
  else if (valueType == orion::ValueTypeString)
  {
    LM_T(LmtLegacy,      ("%s%s (%s)",
                                 indent.c_str(),
                                 name.c_str(),
                                 stringValue.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    LM_T(LmtLegacy,      ("%s%s (%s)",
                                 indent.c_str(),
                                 name.c_str(),
                                 (boolValue == true)? "true" : "false"));
    return;
  }
  else if (valueType == orion::ValueTypeNull)
  {
    LM_T(LmtLegacy,      ("%s%s (null)",
                                 indent.c_str(),
                                 name.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_T(LmtLegacy,      ("%s%s (not given)",
                                 indent.c_str(),
                                 name.c_str()));
    return;
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    LM_T(LmtLegacy,      ("%s%s (%f)",
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
    LM_T(LmtLegacy, ("%sname:      %s",
                                indent.c_str(),
                                name.c_str()));
  }

  LM_T(LmtLegacy, ("%scontainer: %s",
                              indent.c_str(),
                              container->name.c_str()));
  LM_T(LmtLegacy, ("%slevel:     %d",
                              indent.c_str(),
                              level));
  LM_T(LmtLegacy, ("%ssibling:   %d",
                              indent.c_str(),
                              siblingNo));
  LM_T(LmtLegacy, ("%stype:      %s",
                              indent.c_str(),
                              orion::valueTypeName(valueType)));
  LM_T(LmtLegacy, ("%spath:      %s",
                              indent.c_str(),
                              path.c_str()));
  LM_T(LmtLegacy, ("%srootP:     %s",
                              indent.c_str(),
                              rootP->name.c_str()));

  if (valueType == orion::ValueTypeString)
  {
    LM_T(LmtLegacy, ("%sString Value:     %s",
                                indent.c_str(),
                                stringValue.c_str()));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    LM_T(LmtLegacy, ("%sBool Value:     %s",
                                indent.c_str(),
                                (boolValue == false)? "false" : "true"));
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    LM_T(LmtLegacy, ("%sNumber Value:     %f",
                                indent.c_str(),
                                numberValue));
  }
  else if (valueType == orion::ValueTypeNull)
  {
    LM_T(LmtLegacy, ("%sNull",
                                indent.c_str()));
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_T(LmtLegacy, ("%sNotGiven",
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

    LM_T(LmtLegacy, ("%s%lu children (%s)",
                                indent.c_str(),
                                childV.size(),
                                childrenString.c_str()));

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      childV[ix]->show(indent + "  ");
    }
  }

  LM_T(LmtLegacy, (""));
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

        alarmMgr.badInput(orionldState.clientIp, rootP->error);
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
          alarmMgr.badInput(orionldState.clientIp, rootP->error);

          return rootP->error;
        }
      }
    }
  }
  else
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(orionldState.clientIp, "found a forbidden character in the value of an attribute");
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
* render -
*/
std::string CompoundValueNode::render(ApiVersion apiVersion, bool noComma, bool noTag)
{
  std::string  out       = "";
  bool         jsonComma = siblingNo < (int) container->childV.size() - 1;
  std::string  key       = (container->valueType == orion::ValueTypeVector)? "item" : name;

  if (noComma == true)
    jsonComma = false;

  if (apiVersion == API_VERSION_NGSI_V2)
    return toJson(true, false); // FIXME P8: The info on comma-after-or-not is not available here ...


  if (valueType == orion::ValueTypeString)
    out = valueTag(key, stringValue, jsonComma, container->valueType == orion::ValueTypeVector);
  else if (valueType == orion::ValueTypeNumber)
    out = valueTag(key, toString(numberValue), jsonComma, container->valueType == orion::ValueTypeVector, true);
  else if (valueType == orion::ValueTypeBoolean)
    out = valueTag(key, boolValue? "true" : "false", jsonComma, container->valueType == orion::ValueTypeVector, true);
  else if (valueType == orion::ValueTypeNull)
    out = valueTag(key, "null", jsonComma, container->valueType == orion::ValueTypeVector, true);
  else if (valueType == orion::ValueTypeNotGiven)
    out = valueTag(key, "not given", jsonComma, container->valueType == orion::ValueTypeVector, true);
  else if ((valueType == orion::ValueTypeVector) && (container != this))
  {
    out += startTag(container->valueType == orion::ValueTypeObject ? key : "", true);
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(apiVersion);
    }

    out += endTag(jsonComma, true);
  }
  else if ((valueType == orion::ValueTypeVector) && (container == this))
  {
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(apiVersion);
    }
  }
  else if ((valueType == orion::ValueTypeObject) && (container->valueType == orion::ValueTypeVector))
  {
    out += startTag();
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(apiVersion);
    }

    out += endTag(jsonComma, false);
  }
  else if (valueType == orion::ValueTypeObject)
  {
    if (rootP != this)
    {
      if (noTag == false)
        out += startTag(key);

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(apiVersion);
      }

      if (noTag == false)
        out += endTag(jsonComma, false);
    }
    else
    {
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(apiVersion);
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* toJson -
*
* FIXME P3: isLastElement is not used and should be removed
*/
std::string CompoundValueNode::toJson(bool isLastElement, bool comma)
{
  std::string  out       = "";
  bool         jsonComma = false;
  std::string  key       = name;

  if (container != NULL)
  {
    if (!container->childV.empty())
    {
      if (siblingNo < ((int) container->childV.size() - 1))
      {
        jsonComma = true;
      }
    }

    if (container->valueType == orion::ValueTypeVector)
    {
      key = "item";
    }
  }

  // No "comma after" if toplevel
  if ((container == this) || (comma == false))
    jsonComma = false;

  if (valueType == orion::ValueTypeString)
  {
    if (container->valueType == orion::ValueTypeVector)
      out = JSON_STR(stringValue);
    else
      out = JSON_STR(key) + ":" + JSON_STR(stringValue);
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    if (container->valueType == orion::ValueTypeVector)
      out = JSON_NUMBER(toString(numberValue));
    else
      out = JSON_STR(key) + ":" + JSON_NUMBER(toString(numberValue));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    if (container->valueType == orion::ValueTypeVector)
      out = JSON_BOOL(boolValue);
    else
      out = JSON_STR(key) + ":" + JSON_BOOL(boolValue);
  }
  else if (valueType == orion::ValueTypeNull)
  {
    if (container->valueType == orion::ValueTypeVector)
      out = "null";
    else
      out = JSON_STR(key) + ":" + "null";
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_E(("Runtime Error (value not given (%s))", name.c_str()));
    if (container->valueType == orion::ValueTypeVector)
      out = "null";
    else
      out = JSON_STR(key) + ":" + "not given";
  }
  else if ((valueType == orion::ValueTypeVector) && (renderName == true))
  {
    out += JSON_STR(name) + ":[";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(false, true);
    }

    out += "]";
  }
  else if ((valueType == orion::ValueTypeVector) && (container == this))
  {
    //
    // NOTE: Here, the '[]' are already added in the calling function
    //
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(ix == childV.size() - 1, true);
    }
  }
  else if ((valueType == orion::ValueTypeVector) && (container->valueType == orion::ValueTypeVector))
  {
    out += "[";

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(false, true);
    }

    out += "]";
  }
  else if (valueType == orion::ValueTypeVector)
  {
    out += JSON_STR(name) + ":[";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(false, true);
    }

    out += "]";
  }
  else if ((valueType == orion::ValueTypeObject) && (renderName == true))
  {
    if (name == "toplevel")
    {
      name = "value";
    }

    out += JSON_STR(name) + ":{";

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(ix == childV.size() - 1, true);
    }

    out += "}";
  }
  else if ((valueType == orion::ValueTypeObject) && (container->valueType == orion::ValueTypeVector))
  {
    out += "{";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(ix == childV.size() - 1, true);
    }

    out += "}";
  }
  else if (valueType == orion::ValueTypeObject)
  {
    if (rootP != this)
    {
      out += JSON_STR(name) + ":{";

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->toJson(ix == childV.size() - 1, true);
      }

      out += "}";
    }
    else
    {
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->toJson(true, true);
      }
    }
  }

  out += jsonComma? "," : "";

  return out;
}



/* ****************************************************************************
*
* clone -
*/
CompoundValueNode* CompoundValueNode::clone(void)
{
  CompoundValueNode* me;

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
