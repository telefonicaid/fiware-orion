#ifndef SRC_LIB_PARSE_COMPOUNDVALUENODE_H_
#define SRC_LIB_PARSE_COMPOUNDVALUENODE_H_

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
#include <vector>

#include "common/globals.h"

#include "orionTypes/OrionValueType.h"


namespace orion
{
/* ****************************************************************************
*
* CompoundValueNode -
*
* The class fields:
* -------------------------------------------------------------------------------
* o name         Note that not all nodes have a name, not in JSON payloads, nor when
*                getting the info from mongo BSON
*
* o valueType    There are the following types of nodes: Vectors, Objects, Strings, Numbers and Bools
*                The root node is somehow special, but is always either Vector or Object.
*
* o stringValue  The value of a String in the tree.
*
* o numberValue  The value of a Number in the tree.
*
* o boolValue    The value of a Bool in the tree.
*
* o childV       A vector of the children of a Vector or Object.
*                Contains pointers to CompoundValueNode.
*
*/
class CompoundValueNode
{
 public:
  // Tree fields
  std::string                        name;
  orion::ValueType                   valueType;
  std::string                        stringValue;
  double                             numberValue;
  bool                               boolValue;
  std::vector<CompoundValueNode*>    childV;

  // Constructors/Destructors
  CompoundValueNode();
  explicit CompoundValueNode(orion::ValueType _type);

  CompoundValueNode
  (
    const std::string&  _name,
    const std::string&  _value,
    orion::ValueType    _type
  );


  CompoundValueNode
  (
    const std::string&  _name,
    const char*         _value,
    orion::ValueType    _type
  );


  CompoundValueNode
  (
    const std::string&  _name,
    double              _value,
    orion::ValueType    _type
  );

  CompoundValueNode
  (
    const std::string&  _name,
    bool                 _value,
    orion::ValueType    _type
  );

  ~CompoundValueNode();

  CompoundValueNode*  clone(void);
  CompoundValueNode*  add(CompoundValueNode* node);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, const std::string& _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, const char* _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, double _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, bool _value);
  std::string         check(const std::string& path);
  std::string         finish(void);

  std::string         toJson(void);

  void                shortShow(const std::string& indent);
  void                show(const std::string& indent);

  bool                isVector(void);
  bool                isObject(void);
  bool                isString(void);

  const char*         cname(void);
};

}  // namespace orion

#endif  // SRC_LIB_PARSE_COMPOUNDVALUENODE_H_
