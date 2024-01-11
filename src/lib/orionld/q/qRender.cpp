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
#include <stdio.h>                                             // snprintf

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/q/qRender.h"                                 // Own interface



// -----------------------------------------------------------------------------
//
// qRenderOp -
//
// Child 1 (LHS) must be a variable
// Child 2 (RHS) must be a constant (string, int, float, or bool)
//
static bool qRenderOp(QNode* qP, const char* opString, ApiVersion apiVersion, char* buf, int bufLen, bool* mqP, int* bufIxP)
{
  QNode* lhs       = qP->value.children;
  QNode* rhs       = lhs->next;
  char*  bufP      = &buf[*bufIxP];
  int    bytesLeft = bufLen - *bufIxP;
  int    chars;

  switch (rhs->type)
  {
  case QNodeFloatValue:
    chars = snprintf(bufP, bytesLeft, "%s%s%f", lhs->value.v, opString, rhs->value.f);
    break;

  case QNodeIntegerValue:
    chars = snprintf(bufP, bytesLeft, "%s%s%lld", lhs->value.v, opString, rhs->value.i);
    break;

  case QNodeStringValue:
    chars = snprintf(bufP, bytesLeft, "%s%s\"%s\"", lhs->value.v, opString, rhs->value.s);
    break;

  case QNodeTrueValue:
  case QNodeFalseValue:
    chars = snprintf(bufP, bytesLeft, "%s%s%s", lhs->value.v, opString, (rhs->type == QNodeTrueValue)? "true" : "false");
    break;

  case QNodeRegexpValue:
    return false;

  default:
    chars = 0;
  }

  *bufIxP += chars;

  if (*bufIxP >= bufLen)
  {
    LM_W(("Buffer too small for qRender - enlarge and recompile!"));
    return false;
  }

  buf[*bufIxP] = 0;
  return true;
}

// qRenderAND - any number of children

// -----------------------------------------------------------------------------
//
// qRender -
//
static bool qRender(QNode* qP, ApiVersion apiVersion, char* buf, int bufLen, bool* mqP, int* bufIxP)
{
  if ((qP->type == QNodeOr) && (apiVersion == API_VERSION_NGSI_V2))
    return false;

  switch (qP->type)
  {
  case QNodeEQ:         return qRenderOp(qP, "==", apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeNE:         return qRenderOp(qP, "!=", apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeLT:         return qRenderOp(qP, "<",  apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeLE:         return qRenderOp(qP, "<=", apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeGE:         return qRenderOp(qP, ">=", apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeGT:         return qRenderOp(qP, ">",  apiVersion, buf, bufLen, mqP, bufIxP);
#if 0
  case QNodeAnd:        return qRenderAND(qP, apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeOr:         return qRenderOR(qP, apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeExists:     return qRenderExists(qP, apiVersion, buf, bufLen, mqP, bufIxP);
  case QNodeNotExists:  return qRenderNotExists(qP, apiVersion, buf, bufLen, mqP, bufIxP);
#endif
  default:
    return false;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// qRender -
//
bool qRender(QNode* qP, ApiVersion apiVersion, char* buf, int bufLen, bool* mqP)
{
  int bufIx = 0;

  bzero(buf, bufLen);

  if (qRender(qP, apiVersion, buf, bufLen, mqP, &bufIx) == false)
  {
    strncpy(buf, "P;!P", bufLen);
    return false;
  }

  return true;
}
