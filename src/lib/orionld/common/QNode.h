#ifndef SRC_LIB_ORIONLD_COMMON_QNODE_H_
#define SRC_LIB_ORIONLD_COMMON_QNODE_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/



// ----------------------------------------------------------------------------
//
// Forward declarations
//
struct QNode;



// ----------------------------------------------------------------------------
//
// QNodeType - types of tokens in a Q-filter
//
typedef enum QNodeType
{
  QNodeVoid,                     // Nothing, Nada, Zilch
  QNodeOpen,                     // '('
  QNodeClose,                    // ')'
  QNodeAnd,                      // ';'
  QNodeOr,                       // '|'
  QNodeExists,                   // ''
  QNodeNotExists,                // '!'
  QNodeEQ,                       // '=='
  QNodeNE,                       // '!='
  QNodeGE,                       // '>='
  QNodeGT,                       // '>'
  QNodeLE,                       // '<='
  QNodeLT,                       // '<'
  QNodeMatch,                    // '~='
  QNodeNoMatch,                  // '!~='
  QNodeComma,                    // ','
  QNodeRange,                    // '..'
  QNodeVariable,                 // attrName[.attrName]*  ([0-9][a-z[A-Z]_)+
  QNodeFloatValue,               // E.g.: '0.123'
  QNodeIntegerValue,             // E.g.: '512'
  QNodeStringValue,              // E.g.: "this is a string"
  QNodeTrueValue,                // 'true'
  QNodeFalseValue,               // 'false'
  QNodeRegexpValue               // RE(regexp)
} QNodeType;



// ----------------------------------------------------------------------------
//
// QNodeValue -
//
typedef union QNodeValue
{
  struct QNode* children;    // pointer to first child of this container
  char*         v;           // Variable name
  double        f;           // Float value
  long long     i;           // Integer value
  char*         s;           // String
  char*         re;          // Regular Expression
  int           level;       // Parenthesis level
} QNodeValue;



// ----------------------------------------------------------------------------
//
// QNode -
//
typedef struct QNode
{
  struct QNode* next;
  QNodeType     type;
  QNodeValue    value;
} QNode;



// ----------------------------------------------------------------------------
//
// qNode -
//
extern QNode* qNode(QNodeType type);



// ----------------------------------------------------------------------------
//
// qNodeType -
//
extern const char* qNodeType(QNodeType type);

#endif  // SRC_LIB_ORIONLD_COMMON_QNODE_H_
