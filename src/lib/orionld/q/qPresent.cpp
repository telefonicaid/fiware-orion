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

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/q/qPresent.h"                                // Own interface



// -----------------------------------------------------------------------------
//
// qTreePresent -
//
static void qTreePresent(QNode* qP, int indent)
{
  char indentV[100];

  memset(indentV, 0x20202020, sizeof(indentV));
  indentV[indent] = 0;

  if (qP->type == QNodeEQ)
  {
    LM_TMP(("Q:%sEQ:", indentV));
    qTreePresent(qP->value.children, indent+2);
    qTreePresent(qP->value.children->next, indent+2);
  }
  else if (qP->type == QNodeVariable)
    LM_TMP(("Q:%s%s (Variable) (v at %p, qP at %p)", indentV, qP->value.v, qP->value.v, qP));
  else if (qP->type == QNodeIntegerValue)
    LM_TMP(("Q:%s%d (Int)", indentV, qP->value.i));
  else if (qP->type == QNodeFloatValue)
    LM_TMP(("Q:%s%f (Float)", indentV, qP->value.f));
  else if (qP->type == QNodeStringValue)
    LM_TMP(("Q:%s%s (String)", indentV, qP->value.s));
  else if (qP->type == QNodeTrueValue)
    LM_TMP(("Q:%sTRUE (Bool)", indentV));
  else if (qP->type == QNodeFalseValue)
    LM_TMP(("Q:%sFALSE (Bool)", indentV));
  else if (qP->type == QNodeExists)
  {
    LM_TMP(("Q:%s Exists (at %p):", indentV, qP));
    qTreePresent(qP->value.children, indent+2);
  }
  else if (qP->type == QNodeNotExists)
  {
    LM_TMP(("Q:%s Not Exists:", indentV));
    qTreePresent(qP->value.children, indent+2);
  }

  else if (qP->type == QNodeOr)
  {
    LM_TMP(("Q:%sOR:", indentV));
    indent+=2;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
      qTreePresent(childP, indent);
  }
  else if (qP->type == QNodeAnd)
  {
    LM_TMP(("Q:%sAND:", indentV));
    indent+=2;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
      qTreePresent(childP, indent);
  }
  else
    LM_TMP(("Q:%s%s (presentation TBI)", indentV, qNodeType(qP->type)));
}



// -----------------------------------------------------------------------------
//
// qPresent -
//
void qPresent(QNode* qP, const char* what)
{
  LM_TMP(("%s:", what));
  qTreePresent(qP, 0);
}



// -----------------------------------------------------------------------------
//
// qListPresent -
//
void qListPresent(QNode* qP, const char* what)
{
  LM_TMP(("%s:", what));

  int ix = 0;
  while (qP != NULL)
  {
    if (qP->type == QNodeVariable)
      LM_TMP(("  %s (Variable) (v at %p)", qP->value.v, qP->value.v));
    else
      LM_TMP(("  %02d: %s", ix, qNodeType(qP->type)));
    qP = qP->next;
  }
}
