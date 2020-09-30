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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kalloc/kaStrdup.h"                                   // kaStrdup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qLexRender.h"                         // qLexRender - DEBUG
#include "orionld/common/qParse.h"                             // Own interface



// ----------------------------------------------------------------------------
//
// varFix -
//
// - If simple attribute name - all OK
// - If attr.b.c, then 'attr' must be extracted, expanded and then '.md.b.c' appended
// - If attr[b.c], then 'attr' must be extracted, expanded and then '.b.c' appended
//
// After implementing expansion in metadata names, for attr.b.c, 'b' needs expansion also
//
static char* varFix(char* varPath, char** detailsP)
{
  char* cP            = varPath;
  char* attrNameP     = varPath;
  char* firstDotP     = NULL;
  char* secondDotP    = NULL;
  char* startBracketP = NULL;
  char* endBracketP   = NULL;
  char* mdNameP       = NULL;
  char* rest          = NULL;
  char  fullPath[1100];


  //
  // Cases:
  //
  // 1. A                 => single attribute     => attrs.A.value
  // 2. A[B]    (qPath)   => B is inside A.value  => attrs.A.value.B
  // 3. A.B     (mqPath)  => B is a metadata      => attrs.A.md.B.value
  // 4. A.B.C   (mqPath)  => B is a metadata      => attrs.A.md.B.value.C
  //
  // - There can be only one '[' in the path
  // - If '[' is found, then there must be a matching ']'
  //
  // So, we need to know:
  // - attrName
  // - mdName (if NO '[' in path)
  // - rest
  //   - For "A.B.C",  attrName == "A",                rest == "B.C"
  //   - For "A[B.C]", attrName == "A", mdName == "B", rest == "C"
  //   => rest == After first '.'
  //
  while (*cP != 0)
  {
    if (*cP == '.')
    {
      if (firstDotP == NULL)
        firstDotP = cP;
      else if (secondDotP == NULL)
        secondDotP = cP;
    }
    else if (*cP == '[')
    {
      if (startBracketP != NULL)
      {
        *detailsP = (char*) "More than one start brackets found";
        return NULL;
      }
      startBracketP  = cP;
    }
    else if (*cP == ']')
    {
      if (endBracketP != NULL)
      {
        *detailsP = (char*) "More than one end brackets found";
        return NULL;
      }
      endBracketP = cP;
    }

    ++cP;
  }

  //
  // Error handling
  //
  if ((startBracketP != NULL) && (endBracketP == NULL))
  {
    *detailsP = (char*) "missing end bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }
  else if ((startBracketP == NULL) && (endBracketP != NULL))
  {
    *detailsP = (char*) "end bracket but no start bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }
  else if ((firstDotP != NULL) && (startBracketP != NULL) && (firstDotP < startBracketP))
  {
    *detailsP = (char*) "found a dot before a start bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }

  //
  // Now we need to NULL out certain characters:
  //
  // Again, four cases:
  // 1. A
  // 2. A[B]
  // 3. A.B
  // 4. A.B.C
  //
  // - If A:  ((startBracketP == NULL) && (firstDotP == NULL))
  //   - attribute: A
  //   => nothing to NULL our
  //   - fullPath:  A-EXPANDED.value
  //
  // - if A[B]: (startBracketP != NULL)
  //   - attribute: A
  //   - rest:      B
  //   => Must NULL out '[' and ']'
  //   - fullPath:  A-EXPANDED.value.B
  //
  // - if A.B ((startBracketP == NULL) && (firstDotP != NULL) && (secondDotP == NULL))
  //   - attribute:  A
  //   - metadata:   B
  //   => Must NULL out the first dot
  //   - fullPath:  A-EXPANDED.md.B.value
  //
  // - if A.B.C ((startBracketP == NULL) && (firstDotP != NULL) && (secondDotP != NULL))
  //   - attribute:  A
  //   - metadata:   B
  //   - rest:       C
  //   => Must NULL out the first two dots
  //   - fullPath:  A-EXPANDED.md.B.value.C
  //
  int caseNo = 0;

  if ((startBracketP == NULL) && (firstDotP == NULL))
    caseNo = 1;
  else if (startBracketP != NULL)
  {
    *startBracketP = 0;
    *endBracketP   = 0;
    rest           = &startBracketP[1];
    caseNo         = 2;
  }
  else if (firstDotP != NULL)
  {
    if (secondDotP == NULL)
    {
      *firstDotP = 0;
      mdNameP    = &firstDotP[1];
      caseNo     = 3;
    }
    else
    {
      *firstDotP  = 0;
      mdNameP     = &firstDotP[1];
      *secondDotP = 0;
      rest        = &secondDotP[1];
      caseNo = 4;
    }
  }

  if (caseNo == 0)
  {
    *detailsP = (char*) "invalid RHS in Q-filter";
    return NULL;
  }

  //
  // All OK - let's compose ...
  //
  char* longNameP = orionldContextItemExpand(orionldState.contextP, attrNameP, true, NULL);

  //
  // Now 'longNameP' needs to be adjusted forthe DB model, that changes '.' for '=' in the database.
  // If we use 'longNameP', that points to the context-cache, we will destroy the cache. We have to work on a copy
  //
  char longName[512];    // 512 seems like an OK limit for max length of an expanded attribute name
  char mdLongName[512];  // 512 seems like an OK limit for max length of an expanded metadata name

  strncpy(longName, longNameP, sizeof(longName) - 1);

  // Turn '.' into '=' for longName
  char* sP = longName;
  while (*sP != 0)
  {
    if (*sP == '.')
      *sP = '=';
    ++sP;
  }

  //
  // Expand mdName if present
  //
  if (mdNameP != NULL)
  {
    if (strcmp(mdNameP, "observedAt") != 0)  // Don't expand "observedAt", nor ...
    {
      char* mdLongNameP  = orionldContextItemExpand(orionldState.contextP, mdNameP, true, NULL);

      strncpy(mdLongName, mdLongNameP, sizeof(mdLongName) - 1);

      // Turn '.' into '=' for md-longname
      char* sP = mdLongName;
      while (*sP != 0)
      {
        if (*sP == '.')
          *sP = '=';
        ++sP;
      }

      mdNameP = mdLongName;
    }
  }

  if (caseNo == 1)
    snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.value", longName);
  else if (caseNo == 2)
    snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.value.%s", longName, rest);
  else if (caseNo == 3)
    snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.md.%s.value", longName, mdNameP);
  else
    snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.md.%s.value.%s", longName, mdNameP, rest);

  return kaStrdup(&orionldState.kalloc, fullPath);
}



// ----------------------------------------------------------------------------
//
// qNodeAppend - append 'childP' to 'container'
//
static QNode* qNodeAppend(QNode* container, QNode* childP)
{
  QNode* lastP  = container->value.children;
  QNode* cloneP = qNode(childP->type);

  if (cloneP == NULL)
    return NULL;

  cloneP->value = childP->value;
  cloneP->next  = NULL;

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
QNode* qParse(QNode* qLexList, char** titleP, char** detailsP)
{
  QNode*     qNodeV[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
  int        qNodeIx    = 0;
  QNode*     qLexP      = qLexList;
  QNode*     opNodeP    = NULL;
  QNode*     compOpP    = NULL;
  QNode*     prevP      = NULL;
  QNode*     leftP      = NULL;
  QNode*     expressionStart;

  while (qLexP != NULL)
  {
    switch (qLexP->type)
    {
    case QNodeOpen:
      expressionStart = qLexP;

      // Lookup the corresponding ')'
      while ((qLexP != NULL) && ((qLexP->type != QNodeClose) || (qLexP->value.level != expressionStart->value.level)))
      {
        prevP = qLexP;
        qLexP = qLexP->next;
      }

      if (qLexP == NULL)
      {
        *titleP   = (char*) "mismatching parenthesis";
        *detailsP = (char*) "no matching ')'";
        return NULL;
      }

      if (prevP == NULL)
      {
        *titleP   = (char*) "mismatching parenthesis";
        *detailsP = (char*) "no matching ')'";
        return NULL;
      }

      //
      // Now, we have a new "sub-expression".
      // - Make qLexP point to ')'->next
      // - Step over the '('
      // - Remove the ending ')'
      //

      // free(prevP->next);
      prevP->next     = NULL;
      expressionStart = expressionStart->next;

      qNodeV[qNodeIx++] = qParse(expressionStart, titleP, detailsP);
      break;

    case QNodeVariable:
      qLexP->value.v = varFix(qLexP->value.v, detailsP);
      if (qLexP->next == NULL)
      {
        if (compOpP == NULL)
          compOpP = qNode(QNodeExists);
        qNodeAppend(compOpP, qLexP);
        qNodeV[qNodeIx++] = compOpP;
        break;
      }
      // NO BREAK !!!
    case QNodeStringValue:
    case QNodeIntegerValue:
    case QNodeFloatValue:
    case QNodeTrueValue:
    case QNodeFalseValue:
    case QNodeRegexpValue:
      if (compOpP == NULL)  // Left-Hand side
        leftP = qLexP;
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
            *titleP   = (char*) "ngsi-ld query language: mixed types in range";
            *detailsP = (char*) qNodeType(upperLimit->type);
            return NULL;
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
              *titleP   = (char*) "ngsi-ld query language: mixed types in comma-list";
              *detailsP = (char*) qNodeType(qLexP->type);
              return NULL;
            }

            qNodeAppend(commaP, valueP);  // OK to enlist commaP and valueP as qLexP point to after valueP
          }

          //
          // Appending last list item
          //
          QNode* valueP = qLexP;

          qNodeAppend(commaP, valueP);
        }

        //
        // If operator is '!', there is no LEFT side of the expression
        //
        if (compOpP->type != QNodeNotExists)
          qNodeAppend(compOpP, leftP);

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
          *detailsP = (char*) "please use parenthesis to avoid confusions";
          return NULL;
        }
      }
      else
        opNodeP = qLexP;
      break;

    case QNodeEQ:
    case QNodeNE:
    case QNodeGE:
    case QNodeGT:
    case QNodeLE:
    case QNodeLT:
    case QNodeMatch:
    case QNodeNoMatch:
      compOpP = qLexP;
      break;

    case QNodeNotExists:
      compOpP = qLexP;
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
      qNodeAppend(opNodeP, qNodeV[ix]);
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
