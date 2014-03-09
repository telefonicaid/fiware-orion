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

   ::std::string                     root;       // Only for 'top level'
   ::std::string                     path;       // relative path of the node, '/' for 'top level'
   ::std::string                     name;       // Name of the node. Always valid when from XML, not always when from JDON
   ::std::string                     value;      // Only for leaves
   int                               level;      // Nesting level
   int                               siblingNo;  // Order in nesting level (sibling number)
   Type                              type;       // The type of this node
   int                               counter;    // Internal variable necessary during parsing
   ComplexValueNode*                 container;  // Pointer to the direct father
   ComplexValueNode*                 rootP;      // So that all children has quick access to its root container
   ::std::vector<ComplexValueNode*>  childV;     // vector of children



   ComplexValueNode(const char* _root);
   ComplexValueNode(ComplexValueNode* _container, std::string _path, std::string _name, std::string _value, int _siblingNo, Type _type, int _level = -1);

   const char*        typeName(void);
   void               add(ComplexValueNode* node);
   void               add(const Type _type, const std::string& _name, const std::string& _containerPath, const std::string& _value = "");
   ComplexValueNode*  lookup(const char* _path, int callNo = 1);
   void               finish(void);
   void               shortShow(std::string indent);
   void               show(std::string indent);
};

} // namespace orion

#endif
