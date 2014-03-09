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
ComplexValueNode::ComplexValueNode(std::string _root)
{
  LM_T(LmtComplexValue, ("Creating ROOT of ComplexValue tree for '%s'", _root.c_str()));
  root       = _root;
  rootP      = this;
  type       = Struct;
  container  = NULL;
  level      = 0;
  name       = "";
  path       = "/";
  siblingNo  = 0;
  LM_T(LmtComplexValue, ("Created ROOT of ComplexValue tree for '%s'", root.c_str()));
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
std::string ComplexValueNode::finish(void)
{
  shortShow("");

  error = "";
  vectorCheck();

  return error;
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
   
  LM_T(LmtComplexValueAdd, ("Adding '%s' in '%s'. I am '%s'", _name.c_str(), _containerPath.c_str(), name.c_str()));

  if (type == Leaf)
    LM_T(LmtComplexValueAdd, ("Adding Leaf '%s', with value '%s' under '%s'", _name.c_str(), value.c_str(), _containerPath.c_str()));
  else
    LM_T(LmtComplexValueAdd, ("Adding Struct '%s' under '%s'", _name.c_str(), _containerPath.c_str()));

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
ComplexValueNode* ComplexValueNode::lookup(const char* _path)
{
   ComplexValueNode*         node = rootP;  // where the lookup is started
   std::vector<std::string>  pathV;
   int                       depth;

   LM_T(LmtComplexValueLookup, ("Looking for '%s'", _path));
   depth = stringSplit(_path, '/', pathV);

   for (int level = 0; level < depth; ++level)
   {
      bool found = false;

      for (unsigned int ix = 0; ix < node->childV.size(); ++ix)
      {
         LM_T(LmtComplexValueLookup, ("Looking for '%s' - comparing with '%s' %d/%d", pathV[level].c_str(), node->childV[ix]->name.c_str(), ix, node->childV.size()));
         if (node->childV[ix]->name == pathV[level])
         {
            found = true;
            node  = node->childV[ix];
            LM_T(LmtComplexValueLookup, ("found node of level %d: '%s'", level, node->name.c_str()));

            if (level == depth -1)
              return node;
            break;
         }
         else
            LM_T(LmtComplexValueLookup, ("Looking for '%s' - '%s' is not a match (%d/%d)", pathV[level].c_str(), node->childV[ix]->name.c_str(), ix, node->childV.size()));
      }

      if (!found)
      {
        LM_T(LmtComplexValueLookup, ("'%s' not found", _path)); 
        return NULL;
      }
   }

   return NULL;
}



/* ****************************************************************************
*
* shortShow - 
*/
void ComplexValueNode::shortShow(std::string indent)
{
  if (type == Vector)
    PRINTF("%s%s (vector)\n", indent.c_str(), name.c_str());
  else if (type == Struct)
    PRINTF("%s%s (struct)\n", indent.c_str(), name.c_str());
  else
    PRINTF("%s%s (%s)\n", indent.c_str(), name.c_str(), value.c_str());

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



/* ****************************************************************************
*
* vectorCheck - 
*/
void ComplexValueNode::vectorCheck(void)
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

  for (unsigned long ix = 0; ix < childV.size(); ++ix)
    childV[ix]->vectorCheck();
}

}
