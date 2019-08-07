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

#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
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
static char* varFix(void* contextP, char* varPath, char* longName, int longNameLen, char** detailsP)
{
  char* cP            = varPath;
  char* attrNameP     = varPath;
  char* firstDotP     = NULL;
  char* secondDotP    = NULL;
  char* startBracketP = NULL;
  char* endBracketP   = NULL;
  char* mdNameP       = NULL;
  char* rest          = NULL;
  char  fullPath[512];


  LM_TMP(("Q: In varFix: Var PATH == '%s'", varPath));

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
      {
        firstDotP = cP;
        LM_TMP(("Q: firstDot: %s", firstDotP));
      }
      else if (secondDotP == NULL)
      {
        secondDotP = cP;
        LM_TMP(("Q: secondDot: %s", secondDotP));
      }
    }
    else if (*cP == '[')
    {
      if (startBracketP != NULL)
      {
        *detailsP = (char*) "More than one start brackets found";
        return NULL;
      }
      startBracketP  = cP;
      LM_TMP(("Q: startBracketP: %s", startBracketP));
    }
    else if (*cP == ']')
    {
      if (endBracketP != NULL)
      {
        *detailsP = (char*) "More than one end brackets found";
        return NULL;
      }
      endBracketP = cP;
      LM_TMP(("Q: endBracketP: %s", endBracketP));
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
  {
    caseNo = 1;

    LM_TMP(("Q: Case 1: A => nothing to NULL our"));
    LM_TMP(("Q: attrName: '%s'", attrNameP));
  }
  else if (startBracketP != NULL)
  {
    LM_TMP(("Q: Case 2: A[B] => Must NULL out '[' and ']'"));
    *startBracketP = 0;
    *endBracketP   = 0;
    rest           = &startBracketP[1];
    caseNo         = 2;

    LM_TMP(("Q: attrNameP: '%s'", attrNameP));
    LM_TMP(("Q: rest:      '%s'", rest));
  }
  else if (firstDotP != NULL)
  {
    if (secondDotP == NULL)
    {
      LM_TMP(("Q: Case 3: A.B => Must NULL out the first dot"));
      *firstDotP = 0;
      mdNameP    = &firstDotP[1];
      caseNo     = 3;

      LM_TMP(("Q: attrName: '%s'", attrNameP));
      LM_TMP(("Q: mdNameP:  '%s'", mdNameP));
    }
    else
    {
      LM_TMP(("Q: Case 4: A.B.C => Must NULL out the first two dots"));
      *firstDotP  = 0;
      mdNameP     = &firstDotP[1];
      *secondDotP = 0;
      rest        = &secondDotP[1];
      caseNo = 4;

      LM_TMP(("Q: attrName: '%s'", attrNameP));
      LM_TMP(("Q: mdNameP:  '%s'", mdNameP));
      LM_TMP(("Q: rest:     '%s'", rest));
    }
  }
  LM_TMP(("Q: Case: %d", caseNo));

  if (caseNo == 0)
  {
    *detailsP = (char*) "invalid RHS in Q-filter";
    return NULL;
  }

  //
  // All OK - let's compose ...
  //
  orionldUriExpand(orionldState.contextP, attrNameP, longName, longNameLen, detailsP);

  // Turn '.' into '=' for longName
  char* sP = longName;
  while (*sP != 0)
  {
    if (*sP == '.')
      *sP = '=';
    ++sP;
  }

  LM_TMP(("Q: longName: %s", longName));

  if (caseNo == 1)
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.value", longName);
  else if (caseNo == 2)
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.value.%s", longName, rest);
  else if (caseNo == 3)
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.md.%s.value", longName, mdNameP);
  else
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.md.%s.value.%s", longName, mdNameP, rest);

  LM_TMP(("Q: fullPath: %s", fullPath));
  return strdup(fullPath);
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
  QNode*     qNodeV[10];
  int        qNodeIx    = 0;
  QNode*     qLexP      = qLexList;
  QNode*     opNodeP    = NULL;
  QNode*     compOpP    = NULL;
  QNode*     prevP      = NULL;
  QNode*     leftP      = NULL;
  QNode*     expressionStart;
  char       longName[512];

#ifdef DEBUG
    qLexRender(qLexList, orionldState.qDebugBuffer, sizeof(orionldState.qDebugBuffer));
    LM_TMP(("Q: KZ: Parsing LEX list: %s", orionldState.qDebugBuffer));
#endif

  while (qLexP != NULL)
  {
    LM_TMP(("Q: KZ: Treating qNode of type '%s'", qNodeType(qLexP->type)));
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
      LM_TMP(("Q: Setting prev (the '%s' before ending ')') to point to NULL", qNodeType(prevP->type)));
      prevP->next     = NULL;
      expressionStart = expressionStart->next;

      LM_TMP(("Q: Calling qParse recursively for qNodeV[%d]", qNodeIx));
      qNodeV[qNodeIx++] = qParse(expressionStart, titleP, detailsP);

      if (qLexP == NULL)
        LM_TMP(("Q: We're Done!!!"));
      break;

    case QNodeVariable:
      LM_TMP(("Q: KZ: QNodeVariable"));
      qLexP->value.v = varFix(orionldState.contextP, qLexP->value.v, longName, sizeof(longName), detailsP);
      if (qLexP->next == NULL)
      {
        if (compOpP == NULL)
          compOpP = qNode(QNodeExists);
        qNodeAppend(compOpP, qLexP);
        qNodeV[qNodeIx++] = compOpP;
        break;
      }
      // NO BREAK !!!
    case QNodeIntegerValue:
    case QNodeFloatValue:
    case QNodeStringValue:
    case QNodeTrueValue:
    case QNodeFalseValue:
    case QNodeRegexpValue:
      if (compOpP == NULL)  // Left-Hand side
      {
        LM_TMP(("Q: Saving '%s' as left-hand-side", qNodeType(qLexP->type)));
        leftP = qLexP;
      }
      else  // Right hand side
      {
        QNode* rangeP = NULL;
        QNode* commaP = NULL;

        LM_TMP(("Q: ------------------------ Got a value for RHS: %s", qNodeType(qLexP->type)));
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

          LM_TMP(("Q: Peeked and saw a COMMA operator - eating comma-list"));

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
          QNode* valueP = qLexP;
          qNodeAppend(commaP, valueP);
        }

        //
        // If operator is '!', there is no LEFT side of the expression
        //
        if (compOpP->type != QNodeNotExists)
          qNodeAppend(compOpP, leftP);

        LM_TMP(("Q: Creating op-left-right mini-tree for %s", qNodeType(compOpP->type)));

        if (rangeP != NULL)
          qNodeAppend(compOpP, rangeP);
        else if (commaP != NULL)
          qNodeAppend(compOpP, commaP);
        else
          qNodeAppend(compOpP, qLexP);

        LM_TMP(("Q: Adding part-tree to qNodeV, on index %d", qNodeIx));
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
        else
          LM_TMP(("Q: One more opNode found: it is consistent wit hthe previous"));
      }
      else
      {
        opNodeP = qLexP;
        LM_TMP(("Q: Set opNode to %s", qNodeType(opNodeP->type)));
      }
      break;

    case QNodeEQ:
    case QNodeNE:
    case QNodeGE:
    case QNodeGT:
    case QNodeLE:
    case QNodeLT:
    case QNodeMatch:
    case QNodeNoMatch:
      if (compOpP != NULL)
        LM_TMP(("Q: HMMmmmmmm compOpP was not NULL - bug?"));
      compOpP = qLexP;
      LM_TMP(("Q: Got comparison operator - %s", qNodeType(compOpP->type)));
      break;

    case QNodeNotExists:
      compOpP = qLexP;
      LM_TMP(("Q: KZ: Got NotExists operator - set compOpP to reference the NotExists operator"));
      break;

    default:
      LM_TMP(("Q: Unsupported Node Type: %s (%d)", qNodeType(qLexP->type), qLexP->type));
      *titleP   = (char*) "parse error: unsupported node type";
      *detailsP = (char*) qNodeType(qLexP->type);
      return NULL;
      break;
    }

    qLexP = qLexP->next;
    if (qLexP != NULL)
      LM_TMP(("Q: For next loop qLexP now points to a node of type '%s'", qNodeType(qLexP->type)));
    else
      LM_TMP(("Q: That was the last loop. %d nodes added to qNodeV", qNodeIx));
  }

  if (opNodeP != NULL)
  {
    LM_TMP(("Q: Appending %d children", qNodeIx));
    for (int ix = 0; ix < qNodeIx; ix++)
      qNodeAppend(opNodeP, qNodeV[ix]);
    return opNodeP;
  }

  LM_TMP(("Q: No OR/AND used - returning qNodeV[0] (at %p)", qNodeV[0]));
  return qNodeV[0];
}
