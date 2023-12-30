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
extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kalloc/kaStrdup.h"                                   // kaStrdup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"         // orionldSubAttributeExpand
#include "orionld/q/qVariableFix.h"                            // qVariableFix
#include "orionld/q/qClone.h"                                  // qClone
#include "orionld/q/qNodeType.h"                               // qNodeType
#include "orionld/q/qNode.h"                                   // qNode
#include "orionld/q/qRelease.h"                                // qRelease
#include "orionld/q/qListRelease.h"                            // qListRelease
#include "orionld/q/qPresent.h"                                // qListPresent, qTreePresent
#include "orionld/q/qParse.h"                                  // Own interface



// ----------------------------------------------------------------------------
//
// qNodeAppend - append 'childP' to 'container'
//
static QNode* qNodeAppend(QNode* container, QNode* childP, bool clone = true)
{
  QNode* lastP  = container->value.children;
  QNode* cloneP;

  if (clone)
  {
    cloneP = qClone(childP);
    if (cloneP == NULL)
      return NULL;
  }
  else
    cloneP = childP;

  if (lastP == NULL)  // No children
    container->value.children = cloneP;
  else
  {
    while (lastP->next != NULL)
      lastP = lastP->next;

    lastP->next = cloneP;
  }

  return cloneP;
}



// ----------------------------------------------------------------------------
//
// qParse -
//
// Example input strings (s):
// - A1>23
// - P1<1|P2>7
// - (P1<1|P2>7);(A1>23|Q3<0);(R2<=14;F1==7)
// - ((P1<1|P2>7);(A1>23|Q3<0))|(R2<=14;F1==7)
//
// * ';' means '&'
// * On the same parenthesis level, the same op must be used (op: AND|OR)
// *
//
QNode* qParse(QNode* qLexList, QNode* endNodeP, bool forDb, bool qToDbModel, char** titleP, char** detailsP)
{
  QNode*     qNodeV[10]      = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
  int        qNodeIx         = 0;
  QNode*     qLexP           = qLexList;
  QNode*     opNodeP         = NULL;
  QNode*     compOpP         = NULL;
  QNode*     leftP           = NULL;
  QNode*     openP           = NULL;
  QNode*     closeP          = NULL;

  while (qLexP != endNodeP)
  {
    if ((compOpP != NULL) && ((qLexP->type == QNodeTrueValue) || (qLexP->type == QNodeFalseValue)))
    {
      if ((compOpP->type != QNodeEQ) && (compOpP->type != QNodeNE))
      {
        *titleP   = (char*) "Invalid Q-Filter";
        *detailsP = (char*) "invalid operator for boolean value";
        qRelease(compOpP);
        qListRelease(qLexList);
        // Free nodes in qNodeV ?
        if (opNodeP != NULL)
          qRelease(opNodeP);

        return NULL;
      }
    }

    bool isMd;   // Needed for qVariableFix - not used here
    switch (qLexP->type)
    {
    case QNodeOpen:
      // Extract all tokens inside the ( )  (both parenthesis on THE SAME LEVEL)
      openP = qLexP;

      if (openP->next == NULL)
      {
        *titleP   = (char*) "Invalid Q-Filter";
        *detailsP = (char*) "empty parenthesis";
        return NULL;
      }

      // Lookup the corresponding ')' - or error if not present
      closeP = openP->next;
      while (true)
      {
        if ((closeP->type == QNodeClose) && (closeP->value.level == openP->value.level))
          break;

        closeP = closeP->next;
        if (closeP == NULL)
        {
          *titleP   = (char*) "Invalid Q-Filter";
          *detailsP = (char*) "mismatching parenthesis - no matching ')'";
          return NULL;
        }
      }
      qNodeV[qNodeIx++] = qParse(openP->next, closeP, forDb, qToDbModel, titleP, detailsP);

      // Make qLexP point to ')' (will get ->next at the end of the loop)
      qLexP = closeP;
      break;

    case QNodeVariable:
      if (qToDbModel == true)
        qLexP->value.v = qVariableFix(qLexP->value.v, forDb, &isMd, detailsP);

      if ((qLexP->next == NULL) || (qLexP->next->type == QNodeOr) || (qLexP->next->type == QNodeAnd) || (qLexP->next->type == QNodeClose))
      {
        if (compOpP == NULL)
        {
          LM_T(LmtQ, ("Existence for '%s'", qLexP->value.v));
          if ((strcmp(qLexP->value.v, "creDate") == 0) || (strcmp(qLexP->value.v, "modDate") == 0))
          {
            *titleP   = (char*) "Invalid Q-Filter (Cannot use Existence on system attributes)";
            *detailsP = (strcmp(qLexP->value.v, "creDate") == 0)? (char*) "createdAt" : (char*) "modifiedAt";
            return NULL;
          }

          LM_T(LmtQ, ("Existence of '%s'", qLexP->value.v));
          compOpP = qNode(QNodeExists);
        }

        qNodeAppend(compOpP, qLexP);
        qNodeV[qNodeIx++] = compOpP;
        compOpP = NULL;  // Current copmpOpP is taken - need a new one for the next time
        break;  // As there is no "RHS" - EXIST is a unary operator
      }
      // NO BREAK !!!
    case QNodeStringValue:
    case QNodeIntegerValue:
    case QNodeFloatValue:
    case QNodeTrueValue:
    case QNodeFalseValue:
    case QNodeRegexpValue:
      if (compOpP == NULL)  // Still no operatore - this is on the Left-Hand side - saved for later
      {
        leftP = qLexP;
      }
      else  // Right hand side
      {
        QNode* rangeP = NULL;
        QNode* commaP = NULL;

        if ((qLexP->next != NULL) && (qLexP->next->type == QNodeRange))
        {
          QNode*    lowerLimit;
          QNode*    upperLimit;

          lowerLimit = qLexP;        // referencing the lower limit
          qLexP      = qLexP->next;  // step over the lower limit
          rangeP     = qLexP;        // referencing the range
          qLexP      = qLexP->next;  // step over the RANGE operator
          upperLimit = qLexP;        // referencing the upper limit

          if (lowerLimit->type != upperLimit->type)
          {
            //
            // If timestamp, the lower limit has been converted into a FLOAT
            // FIXME: convert the upper limit to FLOAT also !  (qLex)
            // Meanwhile, we accept FLOAT .. STRING
            //
            if ((lowerLimit->type != QNodeFloatValue) || (upperLimit->type != QNodeStringValue))
            {
              *titleP   = (char*) "ngsi-ld query language: mixed types in range";
              *detailsP = (char*) qNodeType(upperLimit->type);
              return NULL;
            }
          }

          qNodeAppend(rangeP, lowerLimit);
          qNodeAppend(rangeP, upperLimit);
        }
        else if ((qLexP->next != NULL) && (qLexP->next->type == QNodeComma))
        {
          QNodeType commaValueType  = qLexP->type;

          commaP = qLexP->next;

          while ((qLexP != NULL) && (qLexP->next != NULL) && (qLexP->next->type == QNodeComma))
          {
            QNode* valueP = qLexP;

            qLexP = qLexP->next;  // point to the comma
            qLexP = qLexP->next;  // point to the next value

            if (qLexP->type != commaValueType)
            {
              // Exception - was a String but is now a Float
              if ((commaValueType != QNodeFloatValue) || (qLexP->type != QNodeStringValue))
              {
                *titleP   = (char*) "ngsi-ld query language: mixed types in comma-list";
                *detailsP = (char*) qNodeType(qLexP->type);
                return NULL;
              }
            }

            qNodeAppend(commaP, valueP);  // OK to enlist commaP and valueP as qLexP point to after valueP
          }

          //
          // Appending last list item
          //
          qNodeAppend(commaP, qLexP);
        }

        //
        // If operator is '!', there is no LEFT side of the expression
        //
        if (compOpP->type != QNodeNotExists)
        {
          qNodeAppend(compOpP, leftP);
        }

        if (rangeP != NULL)
          qNodeAppend(compOpP, rangeP);
        else if (commaP != NULL)
          qNodeAppend(compOpP, commaP);
        else
          qNodeAppend(compOpP, qLexP);

        qNodeV[qNodeIx++] = compOpP;
        compOpP    = NULL;
        leftP      = NULL;
      }
      break;

    case QNodeOr:
    case QNodeAnd:
      if (opNodeP != NULL)
      {
        if (qLexP->type != opNodeP->type)
        {
          *titleP   = (char*) "ngsi-ld query language: mixed AND/OR operators";
          *detailsP = (char*) "please use parenthesis to avoid confusion";
          return NULL;
        }
      }
      else
        opNodeP = qClone(qLexP);
      break;

    case QNodeEQ:
    case QNodeNE:
    case QNodeGE:
    case QNodeGT:
    case QNodeLE:
    case QNodeLT:
    case QNodeMatch:
    case QNodeNoMatch:
      compOpP = qClone(qLexP);
      break;

    case QNodeNotExists:
      compOpP = qClone(qLexP);
      break;

    default:
      *titleP   = (char*) "parse error: unsupported node type";
      *detailsP = (char*) qNodeType(qLexP->type);
      return NULL;
      break;
    }
    qLexP = qLexP->next;
  }

  if (opNodeP != NULL)
  {
    for (int ix = 0; ix < qNodeIx; ix++)
      qNodeAppend(opNodeP, qNodeV[ix], false);  // 3rd param FALSE: qNodeV contains compOp's - all already cloned
    return opNodeP;
  }

  if (qNodeV[0] == NULL)
  {
    *titleP   = (char*) "Parse Error in Q-Expression";
    *detailsP = (char*) "Unfinished statement?";
    return NULL;
  }

  return qNodeV[0];
}
