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
#include <stdlib.h>                                            // malloc

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/q/qLexListRender.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// qVariableFix - from qParse.cpp - FIXME: new module q/qVariableFix.h/cpp
//
extern char* qVariableFix(char* varPath, bool forDb, bool* isMqP, char** detailsP);



// -----------------------------------------------------------------------------
//
// intToString -
//
const char* intToString(QNode* qItemP, char* buf, int bufLen)
{
  snprintf(buf, bufLen - 1, "%lld", qItemP->value.i);
  return buf;
}



// -----------------------------------------------------------------------------
//
// floatToString -
//
const char* floatToString(QNode* qItemP, char* buf, int bufLen)
{
  snprintf(buf, bufLen - 1, "%f", qItemP->value.f);
  return buf;
}



// -----------------------------------------------------------------------------
//
// regexToString -
//
const char* regexToString(QNode* qItemP, char* buf, int bufLen)
{
  return "REGEX";
}



// -----------------------------------------------------------------------------
//
// qLexListRender -
//
char* qLexListRender(QNode* qListP, bool* validInV2P, bool* isMqP)
{
  int    outSize = 512;
  char*  outP    = (char*) malloc(outSize);  // realloc if needed
  int    outIx   = 0;
  char*  detail;

  *validInV2P = true;   // Set to false later if need be (in the switch below)
  *isMqP      = false;  // Set to true later if need be (qVariableFix)

  for (QNode* qItemP = qListP; qItemP != NULL; qItemP = qItemP->next)
  {
    char  buf[32];
    char* bufP = buf;

    switch (qItemP->type)
    {
    case QNodeVoid:         bufP = NULL;                                 break;
    case QNodeOpen:         bufP = (char*) "("; *validInV2P = false;     break;
    case QNodeClose:        bufP = (char*) ")"; *validInV2P = false;     break;
    case QNodeAnd:          bufP = (char*) ";";                          break;
    case QNodeOr:           bufP = (char*) "|"; *validInV2P = false;     break;
    case QNodeExists:       bufP = NULL;                                 break;
    case QNodeNotExists:    bufP = (char*) "!";                          break;
    case QNodeGT:           bufP = (char*) ">";                          break;
    case QNodeLT:           bufP = (char*) "<";                          break;
    case QNodeEQ:           bufP = (char*) "==";                         break;
    case QNodeNE:           bufP = (char*) "!=";                         break;
    case QNodeGE:           bufP = (char*) ">=";                         break;
    case QNodeLE:           bufP = (char*) "<=";                         break;
    case QNodeMatch:        bufP = (char*) "~=";                         break;
    case QNodeNoMatch:      bufP = (char*) "!~=";                        break;
    case QNodeComma:        bufP = (char*) ",";                          break;
    case QNodeRange:        bufP = (char*) "..";                         break;
    case QNodeIntegerValue: intToString(qItemP, buf, sizeof(buf));       break;
    case QNodeFloatValue:   floatToString(qItemP, buf, sizeof(buf));     break;
    case QNodeStringValue:  bufP = qItemP->value.s;                      break;
    case QNodeTrueValue:    bufP = (char*) "true";                       break;
    case QNodeFalseValue:   bufP = (char*) "false";                      break;
    case QNodeRegexpValue:
      regexToString(qItemP, buf, sizeof(buf));
      *validInV2P = false;
      break;
    case QNodeVariable:
      bufP = qVariableFix(qItemP->value.v, false, isMqP, &detail);
      if (bufP == NULL)
      {
        orionldError(OrionldInternalError, "qVariableFix failed", detail, 500);
        return NULL;
      }
      break;
    }

    if (bufP == NULL)
      continue;

    int len = strlen(bufP);
    if (outIx + len >= outSize)
    {
      char* newBuf = (char*) realloc(outP, outSize + 512);

      if (newBuf == NULL)
      {
        free(outP);
        orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List", 500);
        return NULL;
      }
      outP = newBuf;
      outSize += 512;
    }
    strncpy(&outP[outIx], bufP, len);
    outIx += len;
  }

  outP[outIx] = 0;
  return outP;
}
