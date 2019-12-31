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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qLexCheck.h"                          // qLexCheck
#include "orionld/common/qLex.h"                               // Own interface



// -----------------------------------------------------------------------------
//
// qStringPush -
//
static QNode* qStringPush(QNode* prev, char* stringValue)
{
  QNode* qNodeP = qNode(QNodeStringValue);

  qNodeP->value.s = stringValue;

  prev->next = qNodeP;

  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qDateTimePush -
//
static QNode* qDateTimePush(QNode* prev, int64_t dateTime)
{
  QNode* qNodeP = qNode(QNodeIntegerValue);

  qNodeP->value.i = dateTime;

  prev->next = qNodeP;

  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qRegexpPush -
//
static QNode* qRegexpPush(QNode* prev, char* regexpValue)
{
  QNode* qNodeP = qNode(QNodeRegexpValue);

  qNodeP->value.re = regexpValue;

  prev->next = qNodeP;

  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qTermPush -
//
static QNode* qTermPush(QNode* prev, char* term, char** titleP, char** detailsP)
{
  //
  // Trim the term
  //
  while (*term == ' ')
    ++term;

  int termLen = strlen(term);
  while ((termLen >= 1) && term[termLen - 1] == ' ')
  {
    term[termLen - 1] = 0;
    --termLen;
  }

  if (*term != 0)
  {
    //
    // Integer, Float, or variable?
    // Special cases: 'true' & 'false'
    // Or: DateTime?
    //
    int        digits   = 0;
    int        dots     = 0;
    int        others   = 0;
    int        spaces   = 0;
    int        hyphens  = 0;
    char*      sP       = term;
    bool       dateTime = false;
    QNodeType  type;

    if (strncmp(sP, "RE(", 3) == 0)
      return qRegexpPush(prev, &sP[3]);
    else if (strcmp(sP, "true") == 0)
      type = QNodeTrueValue;
    else if (strcmp(sP, "false") == 0)
      type = QNodeFalseValue;
    else
    {
      while (*sP != 0)
      {
        if ((*sP >= '0') && (*sP <= '9'))
          ++digits;
        else if (*sP == '.')
          ++dots;
        else if (*sP == ' ')
          ++spaces;
        else if (*sP == '-')
          ++hyphens;
        else
          ++others;

        ++sP;
      }

      if (others == 0)
      {
        if (hyphens > 0)
        {
          dateTime = true;               // MIGHT be a DateTime
          type     = QNodeIntegerValue;
        }
        else if (dots == 0)
          type = QNodeIntegerValue;
        else if (dots == 1)
          type = QNodeFloatValue;
        else
          type = QNodeVariable;
      }
      else
        type = QNodeVariable;
    }

    QNode* qNodeP = qNode(type);

    if (type == QNodeIntegerValue)
    {
      int64_t dTime;

      if ((dateTime == true) && ((dTime = parse8601Time(term)) == -1))
        dateTime = false;

      if (dateTime == false)
        qNodeP->value.i = strtoul(term, NULL, 10);
      else
        qNodeP->value.i = dTime;
    }
    else if (type == QNodeFloatValue)
      qNodeP->value.f = strtod(term, NULL);
    else if (type == QNodeVariable)
      qNodeP->value.v = term;

    prev->next = qNodeP;
    return qNodeP;
  }

  return prev;
}



// ----------------------------------------------------------------------------
//
// qOpPush -
//
static QNode* qOpPush(QNode* prev, QNodeType type)
{
  QNode* qNodeP = qNode(type);

  prev->next = qNodeP;
  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qLex - lexical analysis of an ngsi-ld Q-filter
//
QNode* qLex(char* s, char** titleP, char** detailsP)
{
  char*  sP          = s;
  QNode  dummy;                   // this 'dummy is only used to not lose the pointer to the first QNode in the list
  QNode* current     = &dummy;
  char*  stringStart = s;
  int    level       = 0;

  *titleP = NULL;

  while (1)
  {
    QNodeType type;

    if (*sP == ' ')
    {
      ++sP;
    }
    else if ((*sP == '<') || (*sP == '>'))
    {
      if (sP[1] == '=')
      {
        type = (*sP == '<')? QNodeLE : QNodeGE;
        *sP = 0;
        ++sP;
      }
      else
        type = (*sP == '<')? QNodeLT : QNodeGT;

      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, type);
      stringStart = sP;
    }
    else if ((*sP == '=') && (sP[1] == '='))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeEQ);
      stringStart = sP;
    }
    else if (*sP == '|')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeOr);
      stringStart = sP;
    }
    else if (*sP == ',')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeComma);
      stringStart = sP;
    }
    else if ((*sP == '.') && (sP[1] == '.'))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeRange);
      stringStart = sP;
    }
    else if ((*sP == '~') && (sP[1] == '='))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeMatch);
      stringStart = sP;
    }
    else if (*sP == '!')
    {
      if (sP[1] == '=')
      {
        type = QNodeNE;
        *sP = 0;
        ++sP;
      }
      else if ((sP[1] == '~') && (sP[2] == '='))
      {
        type = QNodeNoMatch;
        *sP = 0;
        ++sP;
        *sP = 0;
        ++sP;
      }
      else
        type = QNodeNotExists;

      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, type);
      stringStart = sP;
    }
    else if (*sP == ';')  // || (*sP == '&'))
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeAnd);
      stringStart = sP;
    }
    else if (*sP == '(')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeOpen);
      current->value.level = level;
      ++level;
      stringStart = sP;
    }
    else if (*sP == ')')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, titleP, detailsP);
      current = qOpPush(current, QNodeClose);
      --level;
      current->value.level = level;
      stringStart = sP;
    }
    else if (*sP == 0)
    {
      current = qTermPush(current, stringStart, titleP, detailsP);
      break;
    }
    else if (*sP == '"')
    {
      char* start = &sP[1];  // step over "

      *sP = 0;
      ++sP;

      while ((*sP != 0) && (*sP != '"'))
        ++sP;

      if (*sP == 0)
      {
        *titleP = (char*) "ngsi-ld query language: non-terminated string";
        *detailsP = sP;

        return NULL;
      }

      *sP = 0;
      ++sP;
      int64_t   dateTime;
      uint64_t  sLen = (uint64_t) (sP - start - 2);

      if ((sLen > 4) && (start[4] == '-') && ((dateTime = parse8601Time(start)) != -1))
        current = qDateTimePush(current, dateTime);
      else
        current = qStringPush(current, start);
    }
    else if (*sP == ' ')
    {
      ++sP;
    }
    else if ((*sP == 'R') && (sP[1] == 'E') && (sP[2] == '('))
    {
      char  prev  = 0;

      sP = &sP[3];  // step over RE(

      //
      // Find closing ')', counting all '(', ')'
      // Skipping of course escaped parenthesis - "\(", "\)"
      //
      int level = 1;
      while (*sP != 0)
      {
        if (prev != '\\')
        {
          if (*sP == ')')
            --level;
          else if (*sP == '(')
            ++level;
        }

        if (level == 0)
          break;
        prev = *sP;
        ++sP;
      }

      if (*sP == 0)
      {
        *titleP = (char*) "ngsi-ld query language: non-terminated regexp";
        *detailsP = sP;

        return NULL;
      }

      *sP = 0;  // NULLing out ')'
      ++sP;     // And stepping over it
      // PUSH is taken care of later
    }
    else
    {
      //
      // Must be part of a Number, a Variable, or a DateTime
      // Valid characters for:
      // - Number:   a-zA-Z0-9_.
      // - Variable: a-zA-Z0-9_.
      // - DateTime: Z0-9_:.
      //
      if ((*sP >= '0') && (*sP <= '9'))
      {}
      else if ((*sP >= 'a') && (*sP <= 'z'))
      {}
      else if ((*sP >= 'A') && (*sP <= 'Z'))
      {}
      else if ((*sP == '[') || (*sP == ']'))
      {}
      else if (*sP == '_')
      {}
      else if (*sP == '.')
      {}
      else if (*sP == '-')
      {}
      else if (*sP == ':')
      {}
      else
      {
        *titleP = (char*) "ngsi-ld query language: invalid character";
        *detailsP = sP;
        sP[1] = 0;
        return NULL;
      }
      ++sP;
    }
  }

  if (qLexCheck(dummy.next, titleP, detailsP) == false)
    return NULL;

  return dummy.next;
}
