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

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/dateTime.h"                           // dateTimeFromString
#include "orionld/q/qNode.h"                                   // qNode
#include "orionld/q/qNodeType.h"                               // qNodeType
#include "orionld/q/qLexCheck.h"                               // qLexCheck
#include "orionld/q/qPresent.h"                                // qListPresent
#include "orionld/q/qListRelease.h"                            // qListRelease
#include "orionld/q/qLex.h"                                    // Own interface



// -----------------------------------------------------------------------------
//
// qStringPush -
//
static QNode* qStringPush(QNode* prev, char* stringValue)
{
  QNode* qNodeP = qNode(QNodeStringValue);

  LM_T(LmtQ, ("Pushing a String: '%s'", stringValue));

  if (orionldState.useMalloc == false)
    qNodeP->value.s = stringValue;
  else
    qNodeP->value.s = strdup(stringValue);

  prev->next = qNodeP;

  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qDateTimePush -
//
static QNode* qDateTimePush(QNode* prev, double dateTime)
{
  QNode* qNodeP = qNode(QNodeFloatValue);

  LM_T(LmtQ, ("Pushing a timestamp: %f", dateTime));
  qNodeP->value.f = dateTime;

  prev->next = qNodeP;

  return qNodeP;
}



// -----------------------------------------------------------------------------
//
// qTermPush -
//
static QNode* qTermPush(QNode* prev, char* term, bool* lastTermIsTimestampP, char** titleP, char** detailsP)
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

  LM_T(LmtQ, ("term: '%s' (termLen: %d)", term, termLen));

  *lastTermIsTimestampP = false;
  if (termLen >= 9)
  {
    if      (strcmp(&term[termLen - 10], "modifiedAt") == 0)    *lastTermIsTimestampP = true;
    else if (strcmp(&term[termLen -  9], "createdAt")  == 0)    *lastTermIsTimestampP = true;
    else if (strcmp(&term[termLen - 10], "observedAt") == 0)    *lastTermIsTimestampP = true;
  }

  if (*lastTermIsTimestampP == true)
    LM_T(LmtQ, ("Pushing a Timestamp term: '%s'", term));
  else
    LM_T(LmtQ, ("Pushing a term: '%s'", term));

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
    int        colons   = 0;
    int        spaces   = 0;
    int        hyphens  = 0;
    int        Ts       = 0;
    int        Zs       = 0;
    char*      sP       = term;
    bool       dateTime = false;
    QNodeType  type;

    if (strcmp(sP, "true") == 0)
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
        else if (*sP == ':')
          ++colons;
        else if (*sP == 'T')
          ++Ts;
        else if (*sP == 'Z')
          ++Zs;
        else
          ++others;

        ++sP;
      }

      if (others == 0)
      {
        if ((hyphens > 0) && (term[4] == '-'))
          dateTime = true;             // MIGHT be a DateTime

        if (dots == 0)
          type = QNodeIntegerValue;    // no dots - its an Integer
        else if (dots == 1)
          type = QNodeFloatValue;      // one dot - its a Floating point
        else
          type = QNodeVariable;        // more than one dot ... probably an error
      }
      else
        type = QNodeVariable;
    }
    LM_T(LmtQ, ("'%s' seems like a %s", term, qNodeType(type)));

    if ((prev != NULL) && ((prev->type == QNodeMatch) || (prev->type == QNodeNoMatch)))
      type = QNodeRegexpValue;

    QNode* qNodeP = qNode(type);

    if (dateTime == true)
    {
      LM_T(LmtQ, ("'%s' might be a DateTime", term));

      char   errorString[256];
      double dTime = dateTimeFromString(term, errorString, sizeof(errorString));

      if (dTime < 0)
        LM_W(("Invalid DateTime: '%s': %s", term, errorString));
      else
      {
        LM_T(LmtQ, ("term: '%s', dTime: %f", term, dTime));
        qNodeP->value.f = dTime;
        qNodeP->type    = QNodeFloatValue;
      }
    }
    else if (type == QNodeIntegerValue)
      qNodeP->value.i = strtoul(term, NULL, 10);
    else if (type == QNodeFloatValue)
      qNodeP->value.f = strtod(term, NULL);
    else if (type == QNodeVariable)
      qNodeP->value.v = term;
    else if (type == QNodeRegexpValue)
      qNodeP->value.re = term;
    // else ERROR ?

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
QNode* qLex(char* s, bool timestampToFloat, char** titleP, char** detailsP)
{
  QNode  dummy;              // this 'dummy is only used to not lose the pointer to the first QNode in the list
  char*  sP                  = s;
  QNode* current             = &dummy;
  char*  stringStart         = s;
  int    level               = 0;
  bool   lastTermIsTimestamp = false;

  // Initialize the dummy
  dummy.next           = NULL;
  dummy.type           = QNodeVoid;
  dummy.value.children = NULL;

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

      current  = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current  = qOpPush(current, type);

      stringStart = sP;
    }
    else if ((*sP == '=') && (sP[1] == '='))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeEQ);
      stringStart = sP;
    }
    else if (*sP == '|')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeOr);
      stringStart = sP;
    }
    else if (*sP == ',')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeComma);
      stringStart = sP;
    }
    else if ((*sP == '.') && (sP[1] == '.') && (current->type != QNodeMatch) && (current->type != QNodeNoMatch))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeRange);
      stringStart = sP;
    }
    else if ((*sP == '~') && (sP[1] == '='))
    {
      *sP = 0;
      ++sP;
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
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

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, type);
      stringStart = sP;
    }
    else if ((*sP == ';') || (*sP == '&'))
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeAnd);
      stringStart = sP;
    }
    else if (*sP == '(')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeOpen);
      current->value.level = level;
      ++level;
      stringStart = sP;
    }
    else if (*sP == ')')
    {
      *sP = 0;
      ++sP;

      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      current = qOpPush(current, QNodeClose);
      --level;
      current->value.level = level;
      stringStart = sP;
    }
    else if (*sP == 0)
    {
      current = qTermPush(current, stringStart, &lastTermIsTimestamp, titleP, detailsP);
      break;
    }
    else if (*sP == '"')
    {
      char* start = &sP[1];  // step over "

      *sP = 0;
      ++sP;

      while ((*sP != 0) && (*sP != '"'))  // Accept anything inside double quotes
        ++sP;

      if (*sP == 0)
      {
        *titleP = (char*) "ngsi-ld query language: non-terminated string";
        *detailsP = sP;

        if (orionldState.useMalloc == true)
          qListRelease(dummy.next);
        return NULL;
      }

      *sP = 0;
      ++sP;

      LM_T(LmtQ, ("timestampToFloat: %s", (timestampToFloat == true)? "true" : "false"));
      if (timestampToFloat == true)
      {
        char      errorString[256];
        double    dateTime;
        uint64_t  sLen = (uint64_t) (sP - start - 2);

        if ((sLen >= 9) && (start[4] == '-') && ((dateTime = dateTimeFromString(start, errorString, sizeof(errorString))) > 0))
        {
          if (lastTermIsTimestamp)
            current = qDateTimePush(current, dateTime);
          else
            current = qStringPush(current, start);
        }
        else
          current = qStringPush(current, start);
      }
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

        if (orionldState.useMalloc == true)
          qListRelease(dummy.next);
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
      // - Number:   0-9.
      // - Variable: a-zA-Z0-9_.
      // - DateTime: Z0-9_:.
      //
      // Pretty much ALL characters should be accepted ...
      // So, for now, no forbidden char test
      //
      ++sP;
    }
  }

  // qListPresent(dummy.next, NULL, "LL", "Lex List sent to qLexCheck");
  if (qLexCheck(dummy.next, titleP, detailsP) == false)
  {
    if (orionldState.useMalloc == true)
      qListRelease(dummy.next);
    return NULL;
  }

  return dummy.next;
}
