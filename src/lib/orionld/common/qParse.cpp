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
static char* varFix(void* contextP, char* varName, char* longName, int longNameLen, char** detailsP)
{
  char* varNameP = varName;
  bool  qPath    = false;
  bool  mqPath   = false;
  char* rest     = NULL;
  char* alias    = varName;

  LM_TMP(("Q: In varFix"));
  while (*varNameP != 0)
  {
    if (*varNameP == '.')
    {
      mqPath = true;
      break;
    }
    else if (*varNameP == '[')
    {
      qPath = true;
      break;
    }

    ++varNameP;
  }

  if (mqPath == true)
  {
    *varNameP = 0;
    rest = &varNameP[1];
  }
  else if (qPath == true)
  {
    *varNameP = 0;
    ++varNameP;
    rest = varNameP;

    // Null out ending ']'
    while (*varNameP != 0)
    {
      if (*varNameP == ']')
      {
        *varNameP = 0;
        ++varNameP;
        if (*varNameP != 0)
        {
          *detailsP = (char*) "garbage after ending ']' in attribute path";
          return NULL;
        }
        break;
      }
      ++varNameP;
    }
  }

  orionldUriExpand(orionldState.contextP, alias, longName, longNameLen, detailsP);
  LM_TMP(("Q: alias:    %s", alias));
  LM_TMP(("Q: rest:     %s", rest));
  LM_TMP(("Q: expanded: %s", longName));

  // Turn '.' into '=' for longName
  char* sP = longName;
  while (*sP != 0)
  {
    if (*sP == '.')
      *sP = '=';
    ++sP;
  }

  LM_TMP(("Q: expanded: %s", longName));

  char fullPath[512];
  if (qPath == true)
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.%s.value", longName, rest);
  else if (mqPath == true)
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.md.%s.value", longName, rest);
  else
    snprintf(fullPath, sizeof(fullPath), "attrs.%s.value", longName);

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
      if (compOpP == NULL) // Left-Hand side
      {
        LM_TMP(("Q: Saving '%s' as left-hand-side", qNodeType(qLexP->type)));
        leftP = qLexP;
      }
      else // Right hand side
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
          
        {
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
