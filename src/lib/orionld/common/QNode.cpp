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
#include "orionld/common/orionldState.h"                       // Own orionldState
#include "orionld/common/QNode.h"                              // Own interface



// ----------------------------------------------------------------------------
//
// qNode -
//
QNode* qNode(QNodeType type)
{
  if (orionldState.qNodeIx >= QNODE_SIZE)
    return NULL;

  QNode* nodeP = &orionldState.qNodeV[orionldState.qNodeIx++];

  nodeP->type = type;
  nodeP->next = NULL;

  return nodeP;
}



// ----------------------------------------------------------------------------
//
// qNodeType -
//
const char* qNodeType(QNodeType type)
{
  switch (type)
  {
  case QNodeVoid:         return "Void";
  case QNodeOpen:         return "Open";
  case QNodeClose:        return "Close";
  case QNodeAnd:          return "AND";
  case QNodeOr:           return "OR";
  case QNodeExists:       return "Exists";
  case QNodeNotExists:    return "NotExists";
  case QNodeEQ:           return "EQ";
  case QNodeNE:           return "NE";
  case QNodeGE:           return "GE";
  case QNodeGT:           return "GT";
  case QNodeLE:           return "LE";
  case QNodeLT:           return "LT";
  case QNodeMatch:        return "Match";
  case QNodeNoMatch:      return "NoMatch";
  case QNodeComma:        return "Comma";
  case QNodeRange:        return "Range";
  case QNodeVariable:     return "Variable";
  case QNodeFloatValue:   return "FloatValue";
  case QNodeIntegerValue: return "IntegerValue";
  case QNodeStringValue:  return "StringValue";
  case QNodeRegexpValue:  return "RegexpValue";
  case QNodeTrueValue:    return "TRUE";
  case QNodeFalseValue:   return "FALSE";
  }

  return "Invalid QNodeType";
}
