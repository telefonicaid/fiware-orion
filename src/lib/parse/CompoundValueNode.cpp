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
* fermin at tid dot es
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
CompoundValueNode::CompoundValueNode(std::string _root)
{
  root       = _root;
  rootP      = this;
  type       = Struct;
  container  = this;
  level      = 0;
  name       = "";
  path       = "/";
  siblingNo  = 0;
}



/* ****************************************************************************
*
* CompoundValueNode - constructor for all nodes except toplevel
*/
CompoundValueNode::CompoundValueNode(CompoundValueNode* _container, std::string _path, std::string _name, std::string _value, int _siblingNo, Type _type, int _level)
{
  container = _container;
  rootP     = container->rootP;
  name      = _name;
  value     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  type      = _type;
}



/* ****************************************************************************
*
* CompoundValueNode - destructor for all nodes except toplevel
*/
CompoundValueNode::~CompoundValueNode()
{
  LM_T(LmtCompoundValue, ("Destroying node '%s', path '%s'", name.c_str(), path.c_str()));
}



/* ****************************************************************************
*
* typeName - 
*/
const char* CompoundValueNode::typeName(const Type _type)
{
  switch (_type)
  {
  case Leaf:         return "Leaf";
  case Struct:       return "Struct";
  case Vector:       return "Vector";
  }

  return "Unknown";
}



/* ****************************************************************************
*
* finish - 
*/
std::string CompoundValueNode::finish(void)
{
  error = "";

  check();

  return error;
}



/* ****************************************************************************
*
* add - 
*/
CompoundValueNode* CompoundValueNode::add(CompoundValueNode* node)
{
  childV.push_back(node);
  return node;
}



/* ****************************************************************************
*
* add - 
*/
CompoundValueNode* CompoundValueNode::add(const Type _type, const std::string& _name, const std::string& _containerPath, const std::string& _value)
{
  if (type == Leaf)
    LM_T(LmtCompoundValueAdd, ("Adding Leaf '%s', with value '%s' under '%s'", _name.c_str(), value.c_str(), _containerPath.c_str()));
  else
     LM_T(LmtCompoundValueAdd, ("Adding %s '%s' under '%s'", typeName(_type), _name.c_str(), _containerPath.c_str()));

  CompoundValueNode* node = new CompoundValueNode(this, _containerPath + "/" + _name, _name, _value, childV.size(), _type, level + 1);

  return add(node);
}



/* ****************************************************************************
*
* shortShow - 
*/
void CompoundValueNode::shortShow(std::string indent)
{
  if (rootP == this)
    PRINTF("%s%s (toplevel)\n", indent.c_str(), name.c_str());
  else if (type == Vector)
    PRINTF("%s%s (vector)\n", indent.c_str(), name.c_str());
  else if (type == Struct)
    PRINTF("%s%s (struct)\n", indent.c_str(), name.c_str());
  else
    PRINTF("%s%s (%s)\n", indent.c_str(), name.c_str(), value.c_str());

  for (unsigned long ix = 0; ix < childV.size(); ++ix)
    childV[ix]->shortShow(indent + "  ");
}



/* ****************************************************************************
*
* show - 
*/
void CompoundValueNode::show(std::string indent)
{
  if (root != "")
  {
    PRINTF("%s\n", root.c_str());
    indent += "  ";
  }

  if (name != "")
    PRINTF("%sname:    %s\n", indent.c_str(), name.c_str());

  if (value != "")
    PRINTF("%svalue:   %s\n", indent.c_str(), value.c_str());

  PRINTF("%slevel:   %d\n", indent.c_str(), level);
  PRINTF("%ssibling: %d\n", indent.c_str(), siblingNo);
  PRINTF("%stype:    %s\n", indent.c_str(), typeName(type));

  if (childV.size() != 0)
  {
    std::string childrenString;

    for (unsigned long ix = 0; ix < childV.size(); ++ix)
    {
      childrenString += childV[ix]->name;
      if (ix != childV.size() - 1)
        childrenString += ", ";
    }

    PRINTF("%s%lu children (%s)\n", indent.c_str(), childV.size(), childrenString.c_str());

    for (unsigned long ix = 0; ix < childV.size(); ++ix)
      childV[ix]->show(indent + "  ");
  }

  PRINTF("\n");
}



/* ****************************************************************************
*
* check - 
*/
void CompoundValueNode::check(void)
{
  if (type == Vector)
  {
    for (unsigned long ix = 1; ix < childV.size(); ++ix)
    {
      if (childV[ix]->name != childV[0]->name)
      {
        rootP->error = std::string("bad tag-name of vector item: '") + childV[ix]->name + "' (should be '" + childV[0]->name + "')";
        LM_E((rootP->error.c_str()));
        return;
      }
    }
  }
  else if (type == Struct)
  {
    for (unsigned long ix = 0; ix < childV.size() - 1; ++ix)
    {
      for (unsigned long ix2 = ix + 1; ix2 < childV.size(); ++ix2)
      {
        if (childV[ix]->name == childV[ix2]->name)
        {
          rootP->error = std::string("duplicated tag-name: '") + childV[ix]->name + "' in '" + childV[ix]->path + "'";
          LM_E((rootP->error.c_str()));
          return;
        }
      }
    }
  }
  else
    return;

  for (unsigned long ix = 0; ix < childV.size(); ++ix)
    childV[ix]->check();
}



/* ****************************************************************************
*
* render - 
*/
std::string CompoundValueNode::render(Format format, std::string indent)
{
  std::string  out       = "";
  bool         jsonComma = siblingNo < (int) container->childV.size() - 1;

  if (type == Leaf)
  {
    std::string  tagName   = (container->type == Vector)? "vector_item" : name;

    out = valueTag(indent, tagName, value, format, jsonComma, false);
  }
  else if (type == Vector)
  {
    out += startTag(indent, name, "", format, true, false);
    for (unsigned long ix = 0; ix < childV.size(); ++ix)
      out += childV[ix]->render(format, indent + "  ");
    out += endTag(indent, name, format, jsonComma, true, true);
  }
  else if (type == Struct)
  {
    if (rootP != this)
    {
      out += startTag(indent, name, "", format, false, false);

      for (unsigned long ix = 0; ix < childV.size(); ++ix)
        out += childV[ix]->render(format, indent + "  ");

      out += endTag(indent, name, format, jsonComma, false, true);
    }
    else
    {
      for (unsigned long ix = 0; ix < childV.size(); ++ix)
        out += childV[ix]->render(format, indent);
    }
  }

  return out;
}

}
