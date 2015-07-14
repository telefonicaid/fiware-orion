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
#include "rest/ConnectionInfo.h"
#include "parse/CompoundValueNode.h"



namespace orion
{
/* ****************************************************************************
*
* CompoundValueNode - constructor for toplevel 'node'
*/
CompoundValueNode::CompoundValueNode()
{
  rootP      = NULL;
  valueType       = Unknown;
  container  = NULL;
  level      = 0;
  name       = "Unset";
  path       = "Unset";
  siblingNo  = 0;

  LM_T(LmtCompoundValue, ("Created EMPTY compound node at %p", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for toplevel 'node'
*/
CompoundValueNode::CompoundValueNode(Type _type)
{
  rootP      = this;
  valueType       = _type;
  container  = this;
  level      = 0;
  name       = "toplevel";
  path       = "/";
  siblingNo  = 0;

  LM_T(LmtCompoundValue, ("Created TOPLEVEL compound node (a %s) at %p", (valueType == Vector)? "Vector" : "Object", this));
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
  Type                _type,
  int                 _level
)
{
  container = _container;
  rootP     = container->rootP;
  name      = _name;
  stringValue     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  valueType      = _type;

  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          typeName(valueType),
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
  Type                _type,
  int                 _level
)
{
  container = _container;
  rootP     = container->rootP;
  name      = _name;
  stringValue     = std::string(_value);
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  valueType      = _type;

  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          typeName(valueType),
                          this));
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
  double              _value,
  int                 _siblingNo,
  Type                _type,
  int                 _level
)
{
  container = _container;
  rootP     = container->rootP;
  name      = _name;
  numberValue     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  valueType      = _type;

  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          typeName(valueType),
                          this));
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
  bool                _value,
  int                 _siblingNo,
  Type                _type,
  int                 _level
)
{
  container = _container;
  rootP     = container->rootP;
  name      = _name;
  boolValue     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  valueType      = _type;

  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          typeName(valueType),
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

  childV.clear();
}



/* ****************************************************************************
*
* typeName -
*/
const char* CompoundValueNode::typeName(const Type _type)
{
  switch (_type)
  {
  case String:       return "String";
  case Number:       return "Number";
  case Bool:         return "Bool";
  case Object:       return "Object";
  case Vector:       return "Vector";
  case Unknown:      return "Unknown";
  }

  return "Invalid";
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

  if (node->valueType == String)
    LM_T(LmtCompoundValueAdd, ("Adding String '%s', with value '%s' under '%s' (%s)",
                               node->name.c_str(),
                               node->stringValue.c_str(),
                               node->container->path.c_str(),
                               node->container->name.c_str()));
  else
    LM_T(LmtCompoundValueAdd, ("Adding %s '%s' under '%s' (%s)", typeName(node->valueType), node->name.c_str(),
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
  const Type          _type,
  const std::string&  _name,
  const std::string&  _value
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
  const Type          _type,
  const std::string&  _name,
  const char*         _value
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
  const Type          _type,
  const std::string&  _name,
  double              _value
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
  const Type          _type,
  const std::string&  _name,
  bool                _value
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
  if ((rootP == this) && (valueType == Vector))
  {
    LM_F(("%s%s (toplevel vector)", indent.c_str(), name.c_str()));
  }
  else if (rootP == this)
  {
    LM_F(("%s%s (toplevel object)", indent.c_str(), name.c_str()));
  }
  else if (valueType == Vector)
  {
    LM_F(("%s%s (vector)", indent.c_str(), name.c_str()));
  }
  else if (valueType == Object)
  {
    LM_F(("%s%s (object)", indent.c_str(), name.c_str()));
  }
  else if (valueType == String)
  {
    LM_F(("%s%s (%s)", indent.c_str(), name.c_str(), stringValue.c_str()));
    return;
  }
  else if (valueType == Bool)
  {
    LM_F(("%s%s (%s)", indent.c_str(), name.c_str(), (boolValue == true)? "true" : "false"));
    return;
  }
  else if (valueType == Number)
  {
    LM_F(("%s%s (%f)", indent.c_str(), name.c_str(), numberValue));
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
    LM_F(("%sname:      %s", indent.c_str(), name.c_str()));
  }

  LM_F(("%scontainer: %s", indent.c_str(), container->name.c_str()));
  LM_F(("%slevel:     %d", indent.c_str(), level));
  LM_F(("%ssibling:   %d", indent.c_str(), siblingNo));
  LM_F(("%stype:      %s", indent.c_str(), typeName(valueType)));
  LM_F(("%spath:      %s", indent.c_str(), path.c_str()));
  LM_F(("%srootP:     %s", indent.c_str(), rootP->name.c_str()));

  if (valueType == String)
  {
    LM_F(("%sString Value:     %s", indent.c_str(), stringValue.c_str()));
  }
  else if (valueType == Bool)
  {
    LM_F(("%sBool Value:     %s", indent.c_str(), (boolValue == false)? "false" : "true"));
  }
  else if (valueType == Number)
  {
    LM_F(("%sNumber Value:     %f", indent.c_str(), numberValue));
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

    LM_F(("%s%lu children (%s)", indent.c_str(), childV.size(), childrenString.c_str()));

    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      childV[ix]->show(indent + "  ");
    }
  }

  LM_F((""));
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
void CompoundValueNode::check(void)
{
  if (valueType == Vector)
  {
    if (childV.size() == 0)
    {
      return;
    }

    for (uint64_t ix = 1; ix < childV.size(); ++ix)
    {
      if (childV[ix]->name != childV[0]->name)
      {
        rootP->error =
          std::string("bad tag-name of vector item: /") + childV[ix]->name + "/, should be /" + childV[0]->name + "/";

        LM_W(("Bad Input (%s)", rootP->error.c_str()));
        return;
      }
    }
  }
  else if (valueType == Object)
  {
    if (childV.size() == 0)
    {
      return;
    }

    for (uint64_t ix = 0; ix < childV.size() - 1; ++ix)
    {
      for (uint64_t ix2 = ix + 1; ix2 < childV.size(); ++ix2)
      {
        if (childV[ix]->name == childV[ix2]->name)
        {
          rootP->error = std::string("duplicated tag-name: /") + childV[ix]->name + "/ in path: " + path;
          LM_W(("Bad Input (%s)", rootP->error.c_str()));

          return;
        }
      }
    }
  }
  else
  {
    // No check made for Strings
    return;
  }

  // 'recursively' call the check method for all children
  for (uint64_t ix = 0; ix < childV.size(); ++ix)
  {
    childV[ix]->check();
  }
}



/* ****************************************************************************
*
* render -
*/
std::string CompoundValueNode::render(ConnectionInfo* ciP, Format format, const std::string& indent)
{
  std::string  out       = "";
  bool         jsonComma = siblingNo < (int) container->childV.size() - 1;
  std::string  tagName   = (container->valueType == Vector)? "item" : name;

  LM_M(("In render"));
  if (ciP->apiVersion == "v2")
  {
    LM_M(("V2 so toJson instead of render"));
    return toJson(true); // FIXME P8: The info on comma-after-or-not is not available here ...
  }

  LM_M(("Still in render (ciP->apiVersion == %s)", ciP->apiVersion.c_str()));
  if (valueType == String)
  {
    LM_T(LmtCompoundValueRender, ("I am a String (%s)", name.c_str()));
    out = valueTag(indent, tagName, stringValue, format, jsonComma, false, container->valueType == Vector);
  }
  else if ((valueType == Vector) && (container != this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s)", name.c_str()));
    out += startTag(indent, tagName, tagName, format, true, container->valueType == Object, true);
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(ciP, format, indent + "  ");
    }

    out += endTag(indent, tagName, format, jsonComma, true, true);
  }
  else if ((valueType == Vector) && (container == this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s) and my container is TOPLEVEL", name.c_str()));
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(ciP, format, indent);
    }
  }
  else if ((valueType == Object) && (container->valueType == Vector))
  {
    LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is a Vector", name.c_str()));
    out += startTag(indent, "item", "", format, false, false);
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(ciP, format, indent + "  ");
    }

    out += endTag(indent, "item", format, jsonComma, false, true);
  }
  else if (valueType == Object)
  {
    if (rootP != this)
    {
      LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is NOT a Vector", name.c_str()));
      out += startTag(indent, tagName, tagName, format, false, true);

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(ciP, format, indent + "  ");
      }

      out += endTag(indent, tagName, format, jsonComma, false, true);
    }
    else
    {
      LM_T(LmtCompoundValueRender, ("I am the TREE ROOT (%s)", name.c_str()));
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(ciP, format, indent);
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* toJson -
*/
std::string CompoundValueNode::toJson(bool isLastElement)
{
  std::string  out       = "";
  bool         jsonComma = siblingNo < (int) container->childV.size() - 1;
  std::string  tagName   = (container->valueType == Vector)? "item" : name;

  // No "comma after" if toplevel
  if (container == this)
  {
    jsonComma = false;
  }

  LM_M(("In CompoundValueNode::toJson (%s, type %s, jsonComma: %s, siblings: %d)",
        name.c_str(), typeName(valueType), jsonComma? "true" : "false", container->childV.size()));

  if (valueType == String)
  {
    LM_T(LmtCompoundValueRender, ("I am a String (%s)", name.c_str()));
    if (container->valueType == Vector)
    {
      out = JSON_STR(stringValue);
    }
    else
    {
      out = JSON_STR(tagName) + ":" + JSON_STR(stringValue);
    }
  }
  else if (valueType == Number)
  {
    char  num[32];

    LM_T(LmtCompoundValueRender, ("I am a Number (%s)", name.c_str()));
    snprintf(num, sizeof(num), "%f", numberValue);

    if (container->valueType == Vector)
    {
      out = JSON_NUMBER(num);
    }
    else
    {
      out = JSON_STR(tagName) + ":" + JSON_NUMBER(num);
    }
  }
  else if (valueType == Bool)
  {
    LM_T(LmtCompoundValueRender, ("I am a Bool (%s)", name.c_str()));

    if (container->valueType == Vector)
    {
      out = JSON_BOOL(boolValue);
    }
    else
    {
      out = JSON_STR(tagName) + ":" + JSON_BOOL(boolValue);
    }
  }
  else if ((valueType == Vector) && (container != this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s)", name.c_str()));
    out += JSON_STR(name) + ":[";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(false);
    }

    out += "]";
  }
  else if ((valueType == Vector) && (container == this))
  {
    //
    // NOTE: Here, the '[]' are already added in the calling function
    //
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s) and my container is TOPLEVEL", name.c_str()));
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(ix == childV.size() - 1);
    }
  }
  else if ((valueType == Object) && (container->valueType == Vector))
  {
    LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is a Vector", name.c_str()));
    out += "{";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->toJson(ix == childV.size() - 1);
    }

    out += "}";
  }
  else if (valueType == Object)
  {
    if (rootP != this)
    {
      LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is NOT a Vector", name.c_str()));
      out += JSON_STR(name) + ":{";

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->toJson(ix == childV.size() - 1);
      }

      out += "}";
    }
    else
    {
      LM_T(LmtCompoundValueRender, ("I am the TREE ROOT (%s: %d children)", name.c_str(), childV.size()));
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        LM_M(("Calling toJson for child %d: %s", ix, childV[ix]->name.c_str()));
        out += childV[ix]->toJson(true);
        LM_M(("out: %s", out.c_str()));
      }
    }
  }

  out += jsonComma? "," : "";
  LM_M(("out[%s]: %s", name.c_str(), out.c_str()));
  return out;
}



/* ****************************************************************************
*
* clone -
*/
CompoundValueNode* CompoundValueNode::clone(void)
{
  LM_T(LmtCompoundValue, ("cloning '%s'", name.c_str()));
  CompoundValueNode* me;
  if (rootP == this)
  {
    me = new CompoundValueNode(valueType);
  }
  else
  {
    switch (valueType)
    {
    case String:
    case Object:
    case Vector:
      me = new CompoundValueNode(container, path, name, stringValue, siblingNo, valueType, level);
      break;
    case Number:
      me = new CompoundValueNode(container, path, name, numberValue, siblingNo, valueType, level);
      break;
    case Bool:
      me = new CompoundValueNode(container, path, name, boolValue, siblingNo, valueType, level);
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
  return (valueType == Vector);
}



/* ****************************************************************************
*
* isObject -
*/
bool CompoundValueNode::isObject(void)
{
  return (valueType == Object);
}



/* ****************************************************************************
*
* isString -
*/
bool CompoundValueNode::isString(void)
{
  return (valueType == String);
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
* cvalue -
*/
const char* CompoundValueNode::cvalue(void)
{
  return stringValue.c_str();
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
