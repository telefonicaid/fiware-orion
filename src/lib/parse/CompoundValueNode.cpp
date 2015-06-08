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
  type       = Unknown;
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
  type       = _type;
  container  = this;
  level      = 0;
  name       = "toplevel";
  path       = "/";
  siblingNo  = 0;

  LM_T(LmtCompoundValue, ("Created TOPLEVEL compound node (a %s) at %p", (type == Vector)? "Vector" : "Object", this));
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel
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
  value     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  type      = _type;

  LM_T(LmtCompoundValue, ("Created compound node '%s' at level %d, sibling number %d, type %s at %p",
                          name.c_str(),
                          level,
                          siblingNo,
                          typeName(type),
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
    show("");

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

  if (node->type == String)
    LM_T(LmtCompoundValueAdd, ("Adding String '%s', with value '%s' under '%s' (%s)",
                               node->name.c_str(),
                               node->value.c_str(),
                               node->container->path.c_str(),
                               node->container->name.c_str()));
  else
    LM_T(LmtCompoundValueAdd, ("Adding %s '%s' under '%s' (%s)", typeName(node->type), node->name.c_str(),
                               node->container->path.c_str(),
                               node->container->name.c_str()));

  childV.push_back(node);
  return node;
}



/* ****************************************************************************
*
* add -
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
* shortShow -
*/
void CompoundValueNode::shortShow(const std::string& indent)
{
  if ((rootP == this) && (type == Vector))
  {
    LM_F(("%s%s (toplevel vector)", indent.c_str(), name.c_str()));
  }
  else if (rootP == this)
  {
    LM_F(("%s%s (toplevel object)", indent.c_str(), name.c_str()));
  }
  else if (type == Vector)
  {
    LM_F(("%s%s (vector)", indent.c_str(), name.c_str()));
  }
  else if (type == Object)
  {
    LM_F(("%s%s (object)", indent.c_str(), name.c_str()));
  }
  else
  {
    LM_F(("%s%s (%s)", indent.c_str(), name.c_str(), value.c_str()));
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

  if (value != "")
  {
    LM_F(("%svalue:     %s", indent.c_str(), value.c_str()));
  }

  LM_F(("%scontainer: %s", indent.c_str(), container->name.c_str()));
  LM_F(("%slevel:     %d", indent.c_str(), level));
  LM_F(("%ssibling:   %d", indent.c_str(), siblingNo));
  LM_F(("%stype:      %s", indent.c_str(), typeName(type)));
  LM_F(("%spath:      %s", indent.c_str(), path.c_str()));
  LM_F(("%srootP:     %s", indent.c_str(), rootP->name.c_str()));

  if (childV.size() != 0)
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
  if (type == Vector)
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
  else if (type == Object)
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
std::string CompoundValueNode::render(Format format, const std::string& indent)
{
  std::string  out       = "";
  bool         jsonComma = siblingNo < (int) container->childV.size() - 1;
  std::string  tagName   = (container->type == Vector)? "item" : name;

  if (type == String)
  {
    LM_T(LmtCompoundValueRender, ("I am a String (%s)", name.c_str()));
    out = valueTag(indent, tagName, value, format, jsonComma, false, container->type == Vector);
  }
  else if ((type == Vector) && (container != this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s)", name.c_str()));
    out += startTag(indent, tagName, tagName, format, true, container->type == Object, true);
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(format, indent + "  ");
    }

    out += endTag(indent, tagName, format, jsonComma, true, true);
  }
  else if ((type == Vector) && (container == this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s) and my container is TOPLEVEL", name.c_str()));
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(format, indent);
    }
  }
  else if ((type == Object) && (container->type == Vector))
  {
    LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is a Vector", name.c_str()));
    out += startTag(indent, "item", "", format, false, false);
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(format, indent + "  ");
    }

    out += endTag(indent, "item", format, jsonComma, false, true);
  }
  else if (type == Object)
  {
    if (rootP != this)
    {
      LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is NOT a Vector", name.c_str()));
      out += startTag(indent, tagName, tagName, format, false, true);

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(format, indent + "  ");
      }

      out += endTag(indent, tagName, format, jsonComma, false, true);
    }
    else
    {
      LM_T(LmtCompoundValueRender, ("I am the TREE ROOT (%s)", name.c_str()));
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(format, indent);
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
  std::string  tagName   = (container->type == Vector)? "item" : name;

  if (type == String)
  {
    LM_T(LmtCompoundValueRender, ("I am a String (%s)", name.c_str()));
    out = valueTag("", tagName, value, JSON, jsonComma, false, container->type == Vector);
  }
  else if ((type == Vector) && (container != this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s)", name.c_str()));
    out += "[";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(JSON, "");
    }

    out += "]";
  }
  else if ((type == Vector) && (container == this))
  {
    LM_T(LmtCompoundValueRender, ("I am a Vector (%s) and my container is TOPLEVEL", name.c_str()));
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(JSON, "");
    }
  }
  else if ((type == Object) && (container->type == Vector))
  {
    LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is a Vector", name.c_str()));
    out += "{";
    for (uint64_t ix = 0; ix < childV.size(); ++ix)
    {
      out += childV[ix]->render(JSON, "");
    }

    out += "}";
  }
  else if (type == Object)
  {
    if (rootP != this)
    {
      LM_T(LmtCompoundValueRender, ("I am an Object (%s) and my container is NOT a Vector", name.c_str()));
      out += "{";

      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(JSON, "");
      }

      out += "}";
    }
    else
    {
      LM_T(LmtCompoundValueRender, ("I am the TREE ROOT (%s)", name.c_str()));
      for (uint64_t ix = 0; ix < childV.size(); ++ix)
      {
        out += childV[ix]->render(JSON, "");
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* clone -
*/
CompoundValueNode* CompoundValueNode::clone(void)
{
  LM_T(LmtCompoundValue, ("cloning '%s'", name.c_str()));

  CompoundValueNode* me = (rootP == this)? new CompoundValueNode(type) :
    new CompoundValueNode(container, path, name, value, siblingNo, type, level);

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
  return (type == Vector);
}



/* ****************************************************************************
*
* isObject -
*/
bool CompoundValueNode::isObject(void)
{
  return (type == Object);
}



/* ****************************************************************************
*
* isString -
*/
bool CompoundValueNode::isString(void)
{
  return (type == String);
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
  return value.c_str();
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
