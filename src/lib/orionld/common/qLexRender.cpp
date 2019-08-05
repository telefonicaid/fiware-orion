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
#include "orionld/common/qLexRender.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// qValueGet -
//
static int qValueGet(QNode* qLexP, char* buf, int bufLen)
{
  if (qLexP->type == QNodeVariable)
    snprintf(buf, bufLen, "VAR:%s", qLexP->value.v);
  else if (qLexP->type == QNodeIntegerValue)
    snprintf(buf, bufLen, "INT:%lld", qLexP->value.i);
  else if (qLexP->type == QNodeFloatValue)
    snprintf(buf, bufLen, "FLT:%f", qLexP->value.f);
  else if (qLexP->type == QNodeTrueValue)
    strncpy(buf, "TRUE", bufLen);
  else if (qLexP->type == QNodeFalseValue)
    strncpy(buf, "FALSE", bufLen);
  else
    strncpy(buf, qNodeType(qLexP->type), bufLen);

  return strlen(buf);
}



// ----------------------------------------------------------------------------
//
// qLexRender - render the linked list into a char buffer
//
bool qLexRender(QNode* qLexList, char* buf, int bufLen)
{
  QNode*  qLexP = qLexList;
  int     left  = bufLen;
  int     sz;

  if (qLexP == NULL)
    return false;

  sz = qValueGet(qLexP, buf, left);
  left -= sz;

  qLexP = qLexP->next;

  while (qLexP != NULL)
  {
    if (left <= 4)
      return false;

    strcpy(&buf[bufLen - left], " -> ");
    left -= 4;

    sz = qValueGet(qLexP, &buf[bufLen - left], left);
    left -= sz;
    if (left <= 0)
      return false;

    qLexP = qLexP->next;
  }

  return true;
}
