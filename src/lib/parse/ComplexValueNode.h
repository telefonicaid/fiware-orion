#ifndef ORION_COMPLEX_VALUE
#define ORION_COMPLEX_VALUE

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



namespace orion
{

/* ****************************************************************************
*
* - 
*/
class ComplexValueNode
{
public:
   enum Type
   {
      Leaf,
      Struct,
      Vector
   };

   const char* typeName(void)
   {
      switch (type)
      {
      case Leaf:         return "Leaf";
      case Struct:       return "Struct";
      case Vector:       return "Vector";
      }

      return "Unknown";
   }

   // Constructor for toplevel 'node'
   ComplexValueNode(const char* _root)
   {
      LM_T(LmtComplexValue, ("Creating ROOT of ComplexValue tree for '%s'", _root));
      root      = _root;
      type      = Struct;
      container = NULL;
      level     = 0;
      name      = "toplevel";
      path      = "/";
      LM_T(LmtComplexValue, ("Created ROOT of ComplexValue tree for '%s'", root.c_str()));
      show("");
   }

   // Constructor for all nodes except toplevel
   ComplexValueNode(ComplexValueNode* _container, std::string _path, std::string _name, std::string _value, int _siblingNo, Type _type, int _level = -1)
   {
      LM_T(LmtComplexValue, ("Creating node %d of level %d", _siblingNo, _level));

      container = _container;
      if (level == 1)
         rootP = container;
      else
         rootP = container->rootP;

      name      = _name;
      value     = _value;
      path      = _path;
      level     = container->level + 1;
      siblingNo = _siblingNo;
      type      = _type;

      LM_T(LmtComplexValue, ("Created %s node %d of level %d", typeName(), siblingNo, level));

      rootP->show("");
   }

   ::std::string                     root;       // Only for 'top level'
   ::std::string                     path;       // relative path of the node, '/' for 'top level'
   ::std::string                     name;       // Name of the node. Always valid when from XML, not always when from JDON
   ::std::string                     value;      // Only for leaves
   int                               level;      // Nesting level
   int                               siblingNo;  // Order in nesting level (sibling number)
   Type                              type;       // The type of this node
   ComplexValueNode*                 container;  // Pointer to the direct father
   ComplexValueNode*                 rootP;      // So that all children has quick access to its root container
   ::std::vector<ComplexValueNode*>  childV;     // vector of children

   void add(ComplexValueNode* node)
   {
      childV.push_back(node);
   }

   void show(std::string indent)
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
         PRINTF("%s%lu children\n", indent.c_str(), childV.size());
         for (unsigned long ix = 0; ix < childV.size(); ++ix)
            childV[ix]->show(indent + "  ");
      }
   }
};

} // namespace orion

#endif
