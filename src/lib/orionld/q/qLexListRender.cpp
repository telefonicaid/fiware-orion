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
// PUSH_1 -
//
#define PUSH_1(c)          \
do {                       \
  outP[outIx] = c;         \
  ++outIx;                 \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_2 -
//
#define PUSH_2(c1, c2)     \
do {                       \
  outP[outIx]   = c1;      \
  outP[outIx+1] = c2;      \
  outIx += 2;              \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_3 -
//
#define PUSH_3(c1, c2, c3) \
do {                       \
  outP[outIx]   = c1;      \
  outP[outIx+1] = c2;      \
  outP[outIx+2] = c3;      \
  outIx += 3;              \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_VARIABLE -
//
#define PUSH_VARIABLE()                                                                                 \
do {                                                                                                    \
  char* detail;                                                                                         \
  char* varPath  = qVariableFix(qItemP->value.v, false, isMqP, &detail);                                \
                                                                                                        \
  if (varPath == NULL)                                                                                  \
  {                                                                                                     \
      orionldError(OrionldInternalError, "qVariableFix failed", detail, 500);                           \
      return NULL;                                                                                      \
  }                                                                                                     \
                                                                                                        \
  int len = strlen(varPath);                                                                            \
  if (outIx + len >= outSize)                                                                           \
  {                                                                                                     \
    if ((outP = (char*) realloc(outP, outSize + 512)) == NULL)                                          \
    {                                                                                                   \
      orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List Variable", 500);  \
      return NULL;                                                                                      \
    }                                                                                                   \
    outSize += 512;                                                                                     \
  }                                                                                                     \
  strncpy(&outP[outIx], varPath, len + 1);                                                              \
  outIx += len;                                                                                         \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_FLOAT -
//
#define PUSH_FLOAT()                                                                                \
do                                                                                                  \
{                                                                                                   \
  char buf[32];                                                                                     \
  int  len = snprintf(buf, sizeof(buf) - 1, "%f", qItemP->value.f);                                 \
  if (outIx + len >= outSize)                                                                       \
  {                                                                                                 \
    if ((outP = (char*) realloc(outP, outSize + 512)) == NULL)                                      \
    {                                                                                               \
      orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List Text", 500);  \
      return NULL;                                                                                  \
    }                                                                                               \
    outSize += 512;                                                                                 \
  }                                                                                                 \
  strncpy(&outP[outIx], buf, len + 1);                                                              \
  outIx += len;                                                                                     \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_STRING -
//
#define PUSH_STRING()                                                                                   \
do {                                                                                                    \
  int len = strlen(qItemP->value.s);                                                                    \
  if (outIx + len >= outSize)                                                                           \
  {                                                                                                     \
    if ((outP = (char*) realloc(outP, outSize + 512)) == NULL)                                          \
    {                                                                                                   \
      orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List String", 500);    \
      return NULL;                                                                                      \
    }                                                                                                   \
    outSize += 512;                                                                                     \
  }                                                                                                     \
  strncpy(&outP[outIx], qItemP->value.s, len + 1);                                                      \
  outIx += len;                                                                                         \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_INTEGER -
//
#define PUSH_INTEGER()                                                                              \
do {                                                                                                \
  char buf[20];                                                                                     \
  int  len = snprintf(buf, sizeof(buf) - 1, "%lld", qItemP->value.i);                               \
  if (outIx + len >= outSize)                                                                       \
  {                                                                                                 \
    if ((outP = (char*) realloc(outP, outSize + 512)) == NULL)                                      \
    {                                                                                               \
      orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List Text", 500);  \
      return NULL;                                                                                  \
    }                                                                                               \
    outSize += 512;                                                                                 \
  }                                                                                                 \
  strncpy(&outP[outIx], buf, len+1);                                                                \
  outIx += len;                                                                                     \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_BOOLEAN -
//
#define PUSH_BOOLEAN(val)                                                                              \
do {                                                                                                   \
  int len = strlen(val);                                                                               \
  if (outIx + len >= outSize)                                                                          \
  {                                                                                                    \
    if ((outP = (char*) realloc(outP, outSize + 512)) == NULL)                                         \
    {                                                                                                  \
      orionldError(OrionldInternalError, "Out of memory", "allocating room for Q-List Boolean", 500);  \
      return NULL;                                                                                     \
    }                                                                                                  \
    outSize += 512;                                                                                    \
  }                                                                                                    \
  strncpy(&outP[outIx], val, len+1);                                                                   \
  outIx += len;                                                                                        \
} while (0)



// -----------------------------------------------------------------------------
//
// PUSH_REGEX -
//
#define PUSH_REGEX()                             \
do {                                             \
  strncpy(&outP[outIx], "REGEX\0", 6);           \
  outIx += 5;                                    \
} while (0)



// -----------------------------------------------------------------------------
//
// qLexListRender -
//
char* qLexListRender(QNode* qListP, bool* validInV2P, bool* isMqP)
{
  int   outSize = 512;
  char* outP    = (char*) malloc(outSize);  // realloc if needed
  int   outIx   = 0;

  *validInV2P = true;   // Set to false later if need be (in the switch below)
  *isMqP      = false;  // Set to true later if need be (in macro PUSH_VARIABLE)

  int ix = 0;  // DEBUG
  for (QNode* qItemP = qListP; qItemP != NULL; qItemP = qItemP->next)
  {
    switch (qItemP->type)
    {
    case QNodeVoid:                                            break;
    case QNodeOpen:         PUSH_1('('); *validInV2P = false;  break;
    case QNodeClose:        PUSH_1(')'); *validInV2P = false;  break;
    case QNodeAnd:          PUSH_1(';');                       break;
    case QNodeOr:           PUSH_1('|'); *validInV2P = false;  break;
    case QNodeExists:                                          break;
    case QNodeNotExists:    PUSH_1('!');                       break;
    case QNodeGT:           PUSH_1('>');                       break;
    case QNodeLT:           PUSH_1('<');                       break;
    case QNodeEQ:           PUSH_2('=', '=');                  break;
    case QNodeNE:           PUSH_2('=', '=');                  break;
    case QNodeGE:           PUSH_2('>', '=');                  break;
    case QNodeLE:           PUSH_2('<', '=');                  break;
    case QNodeMatch:        PUSH_2('~', '=');                  break;
    case QNodeNoMatch:      PUSH_3('!', '~', '=');             break;
    case QNodeComma:        PUSH_1(',');                       break;
    case QNodeRange:        PUSH_2('.', '.');                  break;
    case QNodeVariable:     PUSH_VARIABLE();                   break;
    case QNodeFloatValue:   PUSH_FLOAT();                      break;
    case QNodeStringValue:  PUSH_STRING();                     break;
    case QNodeIntegerValue: PUSH_INTEGER();                    break;
    case QNodeTrueValue:    PUSH_BOOLEAN("true");              break;
    case QNodeFalseValue:   PUSH_BOOLEAN("false");             break;
    case QNodeRegexpValue:  PUSH_REGEX(); *validInV2P = false; break;
    }
    ++ix;
  }

  return outP;
}
