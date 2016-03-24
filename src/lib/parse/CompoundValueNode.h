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

#include "common/Format.h"
#include "orionTypes/OrionValueType.h"

struct ConnectionInfo;


namespace orion
{
/* ****************************************************************************
*
* CompoundValueNode -
*
* The class fields:
* -------------------------------------------------------------------------------
* o name         When parsing an XML payload, each node in the tree has a tag.
*                The name of the node is taken from the tag-name.
*                When parsing a JSON payload, not necessarily all nodes have a
*                tag, so 'name' can be empty.
*                Also, when creating the tree from mongo BSON, there will often
*                be no 'name', just like the case of JSON payload parsing.
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
* o container    A pointer to the father of the node. The father is the Object/Vector node
*                that owns this node.
*
* o rootP        A pointer to the owner of the entire tree
*
* o error        This string is used by the 'check' function to save any errors detected during
*                the 'check' phase.
*                FIXME P1: May be removed if the check function is modified.
*
* o path         Absolute path of the node in the tree.
*                Used for error messages, e.g. duplicated tag-name in a struct.
*
* o level        The depth or nesting level in which this node lives.
*
* o siblingNo:   This field is used for rendering JSON. It tells us whether a comma should
*                be added after a field (a comma is added unless the sibling number is
*                equal to the number of siblings (the size of the containers child vector).
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


  // Auxiliar fields for creation of the tree
  CompoundValueNode*                 container;
  CompoundValueNode*                 rootP;
  std::string                        error;

  // Needed for JSON rendering
  int                                siblingNo;

  // Fields that may not be necessary
  // FIXME P4: when finally sure, remove the unnecessary fields
  std::string                        path;
  int                                level;

  // Constructors/Destructors
  CompoundValueNode();
  explicit CompoundValueNode(orion::ValueType _type);

  CompoundValueNode
  (
    CompoundValueNode*  _container,
    const std::string&  _path,
    const std::string&  _name,
    const std::string&  _value,
    int                 _siblingNo,
    orion::ValueType    _type,
    int                 _level = -1
  );


  CompoundValueNode
  (
    CompoundValueNode*  _container,
    const std::string&  _path,
    const std::string&  _name,
    const char*         _value,
    int                 _siblingNo,
    orion::ValueType    _type,
    int                 _level = -1
  );


  CompoundValueNode
  (
    CompoundValueNode*  _container,
    const std::string&  _path,
    const std::string&  _name,
    double              _value,
    int                 _siblingNo,
    orion::ValueType    _type,
    int                 _level = -1
  );

  CompoundValueNode
  (
    CompoundValueNode*  _container,
    const std::string&  _path,
    const std::string&  _name,
    bool                 _value,
    int                 _siblingNo,
    orion::ValueType    _type,
    int                 _level = -1
  );

  ~CompoundValueNode();

  CompoundValueNode*  clone(void);
  CompoundValueNode*  add(CompoundValueNode* node);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, const std::string& _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, const char* _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, double _value);
  CompoundValueNode*  add(const orion::ValueType _type, const std::string& _name, bool _value);
  void                check(void);
  std::string         finish(void);
  std::string         render(ConnectionInfo* ciP, const std::string& indent);
  std::string         toJson(bool isLastElement);

  void                shortShow(const std::string& indent);
  void                show(const std::string& indent);

  bool                isVector(void);
  bool                isObject(void);
  bool                isString(void);

  const char*         cname(void);
  const char*         cpath(void);
};

}  // namespace orion

#endif  // SRC_LIB_PARSE_COMPOUNDVALUENODE_H_
