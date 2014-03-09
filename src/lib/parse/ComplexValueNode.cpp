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

#include "parse/ComplexValueNode.h"



namespace orion
{


/* ****************************************************************************
*
* ComplexValueNode - constructor for toplevel 'node' 
*/
ComplexValueNode::ComplexValueNode(const char* _root)
{
  LM_T(LmtComplexValue2, ("Creating ROOT of ComplexValue tree for '%s'", _root));
  root       = _root;
  rootP      = this;
  type       = Struct;
  container  = NULL;
  level      = 0;
  name       = "";
  path       = "/";
  siblingNo  = 0;
  LM_T(LmtComplexValue2, ("Created ROOT of ComplexValue tree for '%s'", root.c_str()));
}



/* ****************************************************************************
*
* ComplexValueNode - constructor for all nodes except toplevel
*/
ComplexValueNode::ComplexValueNode(ComplexValueNode* _container, std::string _path, std::string _name, std::string _value, int _siblingNo, Type _type, int _level)
{
  container = _container;
  rootP     = (level == 1)? container : container->rootP;
  name      = _name;
  value     = _value;
  path      = _path;
  level     = container->level + 1;
  siblingNo = _siblingNo;
  type      = _type;
  counter   = 0;
}



/* ****************************************************************************
*
* typeName - 
*/
const char* ComplexValueNode::typeName(void)
{
  switch (type)
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
void ComplexValueNode::finish(void)
{
  // Detect vectors?
  shortShow("");
}



/* ****************************************************************************
*
* add - 
*/
void ComplexValueNode::add(ComplexValueNode* node)
{
  childV.push_back(node);
}



/* ****************************************************************************
*
* add - 
*/
void ComplexValueNode::add(const Type _type, const std::string& _name, const std::string& _containerPath, const std::string& _value)
{
  ComplexValueNode* owner;
   
  LM_T(LmtComplexValue, ("Adding '%s' in '%s'. I am '%s'", _name.c_str(), _containerPath.c_str(), name.c_str()));

  if (type == Leaf)
    LM_T(LmtComplexValue, ("Adding Leaf '%s', with value '%s' under '%s'", _name.c_str(), value.c_str(), _containerPath.c_str()));
  else
    LM_T(LmtComplexValue, ("Adding Struct '%s' under '%s'", _name.c_str(), _containerPath.c_str()));

  if (_containerPath == name)
    owner = this;
  else
  {
    owner = lookup(_containerPath.c_str());

    if (owner == NULL)
      LM_RVE(("Cannot find Complex Value container '%s'", _containerPath.c_str()));
  }

  ComplexValueNode* node = new ComplexValueNode(owner, _containerPath + "/" + _name, _name, _value, owner->childV.size(), _type, owner->level + 1);
  owner->add(node);
}



/* ****************************************************************************
*
* lookup - 
*/
ComplexValueNode* ComplexValueNode::lookup(const char* _path, int callNo)
{
  std::vector<std::string> pathV;
  int                      depth;
  char*                    lookFor;
  std::string              nextPath;
  
  if (callNo == 1)
  {
    LM_T(LmtComplexValue, ("--------------------------------------------"));
    LM_T(LmtComplexValue, ("  Looking for '%s', starting in '%s'", _path, name.c_str()));
  }
  else
  {
    LM_T(LmtComplexValue, ("  -----   Call %d --------------------", callNo));
    LM_T(LmtComplexValue, ("  Looking for '%s', now we're in '%s'", _path, name.c_str()));
  }

  depth   = stringSplit(_path, '/', pathV);
  LM_T(LmtComplexValue, ("'%s' has depth %d", _path, depth));
  lookFor = (char*) pathV[0].c_str();
     
  LM_T(LmtComplexValue, ("Looking for '%s' in '%s' (depth: %d). This time: '%s'", _path, path.c_str(), depth, lookFor));

  for (unsigned int ix = 0; ix < childV.size(); ++ix)
  {
    if (childV[ix]->name != lookFor)
      continue;

    LM_T(LmtComplexValue, ("Found child '%s'", childV[ix]->name.c_str()));
    if (depth == 1)
      return childV[ix];

    pathV.erase(pathV.begin());

    nextPath = "";
    for (unsigned int pix; pix < pathV.size(); ++pix)
    {
      nextPath += pathV[pix];
      if (pix != pathV.size() - 1)
        nextPath += "/";
    }

    LM_T(LmtComplexValue, ("'Recursive' call for '%s': path: '%s'", childV[ix]->name.c_str(), nextPath.c_str()));
    return childV[ix]->lookup(nextPath.c_str(), callNo + 1);
  }

  return NULL;
}



/* ****************************************************************************
*
* shortShow - 
*/
void ComplexValueNode::shortShow(std::string indent)
{
  PRINTF("%s%s\n", indent.c_str(), name.c_str());
  for (unsigned long ix = 0; ix < childV.size(); ++ix)
    childV[ix]->shortShow(indent + "    ");
}



/* ****************************************************************************
*
* show - 
*/
void ComplexValueNode::show(std::string indent)
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
  PRINTF("%stype:    %s\n", indent.c_str(), typeName());

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

}
