/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qTreePresent.h"                       // Own interface



static const char* indentV[] = {
  "",
  "  ",
  "    ",
  "      ",
  "        ",
  "          ",
  "            ",
  "              ",
  "                "
};



// ----------------------------------------------------------------------------
//
// qTreePresent - DEBUG function to see an entire QNode Tree in the log file
//
void qTreePresent(QNode* qNodeP, int level)
{
  const char* indent = indentV[level];

  if      (qNodeP->type == QNodeVariable)       LM_TMP(("Q: %sVAR: '%s'",   indent, qNodeP->value.v));
  else if (qNodeP->type == QNodeIntegerValue)   LM_TMP(("Q: %sINT: %lld",   indent, qNodeP->value.i));
  else if (qNodeP->type == QNodeFloatValue)     LM_TMP(("Q: %sFLT: %f",     indent, qNodeP->value.f));
  else if (qNodeP->type == QNodeStringValue)    LM_TMP(("Q: %sSTR: '%s'",   indent, qNodeP->value.s));
  else if (qNodeP->type == QNodeTrueValue)      LM_TMP(("Q: %sTRUE",        indent));
  else if (qNodeP->type == QNodeFalseValue)     LM_TMP(("Q: %sFALSE",       indent));
  else if (qNodeP->type == QNodeRegexpValue)    LM_TMP(("Q: %sRE: '%s'",    indent, qNodeP->value.re));
  else
  {
    ++level;
    LM_TMP(("Q: %s%s", indent, qNodeType(qNodeP->type)));
    for (QNode* childP = qNodeP->value.children; childP != NULL; childP = childP->next)
      qTreePresent(childP, level);
  }
}
