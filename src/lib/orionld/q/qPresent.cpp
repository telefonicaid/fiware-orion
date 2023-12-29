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
#include <string.h>                                            // memset

#include "logMsg/logMsg.h"                                     // TraceLevels, LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/q/qNodeType.h"                               // qNodeType
#include "orionld/q/qPresent.h"                                // Own interface



// -----------------------------------------------------------------------------
//
// qTreePresent -
//
static void qTreePresent(QNode* qP, int indent, const char* prefix, TraceLevels tLevel)
{
  if (lmTraceIsSet(LmtQ) == false)
    return;

  char indentV[100];

  memset(indentV, 0x20202020, sizeof(indentV));
  indentV[indent+1] = 0;

  if (qP->type == QNodeEQ)
  {
    LM_T(tLevel, ("%s:%sEQ:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeNE)
  {
    LM_T(tLevel, ("%s:%sNE:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeLT)
  {
    LM_T(tLevel, ("%s:%sLT:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeLE)
  {
    LM_T(tLevel, ("%s:%sLE:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeGT)
  {
    LM_T(tLevel, ("%s:%sGT:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeGE)
  {
    LM_T(tLevel, ("%s:%sGE:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
    qTreePresent(qP->value.children->next, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeVariable)
    LM_T(tLevel, ("%s:%s%s (Variable) (v at %p, qP at %p)", prefix, indentV, qP->value.v, qP->value.v, qP));
  else if (qP->type == QNodeIntegerValue)
    LM_T(tLevel, ("%s:%s%d (Int)", prefix, indentV, qP->value.i));
  else if (qP->type == QNodeFloatValue)
    LM_T(tLevel, ("%s:%s%f (Float)", prefix, indentV, qP->value.f));
  else if (qP->type == QNodeStringValue)
    LM_T(tLevel, ("%s:%s%s (String) at %p (String at %p)", prefix, indentV, qP->value.s, qP, qP->value.s));
  else if (qP->type == QNodeTrueValue)
    LM_T(tLevel, ("%s:%sTRUE (Bool)", prefix, indentV));
  else if (qP->type == QNodeFalseValue)
    LM_T(tLevel, ("%s:%sFALSE (Bool)", prefix, indentV));
  else if (qP->type == QNodeExists)
  {
    LM_T(tLevel, ("%s:%s Exists (at %p):", prefix, indentV, qP));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeNotExists)
  {
    LM_T(tLevel, ("%s:%s Not Exists:", prefix, indentV));
    qTreePresent(qP->value.children, indent+2, prefix, tLevel);
  }
  else if (qP->type == QNodeOr)
  {
    LM_T(tLevel, ("%s:%sOR:", prefix, indentV));
    indent+=2;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
      qTreePresent(childP, indent, prefix, tLevel);
  }
  else if (qP->type == QNodeAnd)
  {
    LM_T(tLevel, ("%s:%sAND:", prefix, indentV));
    indent+=2;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
      qTreePresent(childP, indent, prefix, tLevel);
  }
  else
    LM_T(tLevel, ("%s:%s%s (presentation TBI)", prefix, indentV, qNodeType(qP->type)));
}



// -----------------------------------------------------------------------------
//
// qPresent -
//
void qPresent(QNode* qP, const char* prefix, const char* what, TraceLevels tLevel)
{
  LM_T(tLevel, ("%s: --------------------- %s -----------------------------------", prefix, what));
  qTreePresent(qP, 0, prefix, tLevel);
  LM_T(tLevel, ("%s: --------------------------------------------------------", prefix));
}



// -----------------------------------------------------------------------------
//
// qListPresent -
//
void qListPresent(QNode* qP, QNode* endP, const char* prefix, const char* what)
{
  LM_T(LmtQ, ("%s: %s:", prefix, what));
  LM_T(LmtQ, ("%s: --------------------------------------------------------", prefix));

  int ix = 0;
  while (qP != endP)
  {
    bool op = (qP->type == QNodeGE) || (qP->type == QNodeGT) || (qP->type == QNodeLE) || (qP->type == QNodeLT) || (qP->type == QNodeNE) || (qP->type == QNodeEQ);

    if      (qP->type == QNodeVariable)      LM_T(LmtQ, ("%s:  %02d: Variable (at %p) (var-name '%s' at %p)",        prefix, ix, qP, qP->value.v, qP->value.v));
    else if (qP->type == QNodeIntegerValue)  LM_T(LmtQ, ("%s:  %02d: IntegerValue (at %p): %d",                      prefix, ix, qP, qP->value.i));
    else if (qP->type == QNodeFloatValue)    LM_T(LmtQ, ("%s:  %02d: %f (FloatValue)",                               prefix, ix, qP, qP->value.f));
    else if (qP->type == QNodeStringValue)   LM_T(LmtQ, ("%s:  %02d: StringValue (at %p) (string-value '%s' at %p)", prefix, ix, qP, qP->value.s, qP->value.s));
    else if (qP->type == QNodeAnd)           LM_T(LmtQ, ("%s:  %02d: AND Operator",                                  prefix, ix));
    else if (qP->type == QNodeOr)            LM_T(LmtQ, ("%s:  %02d: OR Operator",                                   prefix, ix));
    else if (qP->type == QNodeExists)        LM_T(LmtQ, ("%s:  %02d: EXISTS Operator",                               prefix, ix));
    else if (qP->type == QNodeNotExists)     LM_T(LmtQ, ("%s:  %02d: NOT-EXISTS Operator",                           prefix, ix));
    else if (qP->type == QNodeRegexpValue)   LM_T(LmtQ, ("%s:  %02d: RegexpValue '%s'",                              prefix, ix, qP->value.v));
    else if (op == true)                     LM_T(LmtQ, ("%s:  %02d: %s Operator",                                   prefix, ix, qNodeType(qP->type)));
    else                                     LM_T(LmtQ, ("%s:  %02d: %s (at %p)",                                    prefix, ix, qNodeType(qP->type), qP));
    qP = qP->next;
    ++ix;
  }

  LM_T(LmtQ, ("%s: --------------------------------------------------------", prefix));
}
