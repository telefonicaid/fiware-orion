#ifndef SRC_LIB_ORIONLD_TYPES_TREENODE_H_
#define SRC_LIB_ORIONLD_TYPES_TREENODE_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
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



// -----------------------------------------------------------------------------
//
// Characteristics of a payload field
//
#define IGNORE           1 << 0
#define MANDATORY        1 << 1
#define IS_URI           1 << 2
#define NOT_SUPPORTED    1 << 3
#define NOT_IMPLEMENTED  1 << 4



// -----------------------------------------------------------------------------
//
// TreeNode -
//
// This type is used as output from pCheckQuery, for POST Query, and it gives us
// direct pointers (knowing the index) to the fields in the payload body.
//
// The implementation is an attempt to improve the pCheck routines.
//
typedef struct TreeNode
{
  const char*  name;
  const char*  longName;
  KjNode*      nodeP;
  int          nodeType;  // bitmask of allowed KjValueType
  int          aux;       // IGNORE, MANDATORY
  void*        output;    // Some pCheck functions generate output (e.g. pCheckQ)
} TreeNode;

#endif  // SRC_LIB_ORIONLD_TYPES_TREENODE_H_
