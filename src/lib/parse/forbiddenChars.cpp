/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "parse/forbiddenChars.h"



/* ****************************************************************************
*
* commonForbidden
*/
inline static bool commonForbidden(char c)
{
  switch (c)
  {
  case '<':
  case '>':
  case '"':
  case '\'':
  case '=':
  case ';':
  case '(':
  case ')':
    return true;
  }
  return false;
}



/* ****************************************************************************
*
* forbiddenChars - 
*/
bool forbiddenChars(const char* s, const char* exceptions)
{
  if (s == (void*) 0)
  {
    return false;
  }

  while (*s != 0)
  {
    if ((exceptions != NULL) && (strchr(exceptions, *s) != NULL))
    {
      ++s;
      continue;
    }

    if (commonForbidden(*s))
    {
      return true;
    }

    ++s;
  }

  return false;
}



/* ****************************************************************************
*
* forbiddenIdChars -
*/
bool forbiddenIdChars(int api, const char* s, const char* exceptions)
{
  if (api == 1 && !checkIdv1)
  {
    return forbiddenChars(s, exceptions);  // old behavior
  }

  return forbiddenIdCharsV2(s, exceptions);
}



/* ****************************************************************************
*
* forbiddenIdCharsV2 -
*/
bool forbiddenIdCharsV2(const char* s, const char* exceptions)
{
  if (s == (void*) 0)
  {
    return false;
  }

  while (*s != 0)
  {
    if ((exceptions != NULL) && (strchr(exceptions, *s) != NULL))
    {
      ++s;
      continue;
    }

    if (*s >= 127 || *s <= 32)
    {
      return true;
    }

    switch (*s)
    {
    case '?':
    case '/':
    case '#':
    case '&':
      return true;
    }

    // Plus common set of forbidden chars
    if(commonForbidden(*s))
    {
      return true;
    }

    ++s;
  }

  return false;
}



/* ****************************************************************************
*
* forbiddenQuotes - any unauthorized quotes?
*
* Quotes (') are used to delimit attribute/metadata/compound-node names that contain a dot (.).
*
* Example (of metadata, but valid also for attribute name and compound node name):
*   We have an attribute A with a metadata called "M.x" (without double-quotes - 3 chars).
*   For a string filter (mq) on M.x, we need to use quotes.
*   A.M.x is interpreted as attribute A, metadata M, compound node x
*   A.'M.x' is what we need to reach the metadata M.x.
*
* So, quotes are allowed as first and last character, and if there is a dot
* before or after the quote.
*
* If a quote is found elsewhere, an error should be returned.
*/
bool forbiddenQuotes(char* s)
{
  int ix = 0;

  while (s[ix] != 0)
  {
    ++ix;  // Remember, quote is allowed as first char, ok to step over ix==0

    if (s[ix] == '\'')
    {
      if      (s[ix - 1] == '.')  {}  // OK - a.'x.b' is allowed (see quote in pos 2 - dot before)
      else if (s[ix + 1] == '.')  {}  // OK - a.'x.b'.c is allowed (see quote in pos 6 - dot after)
      else if (s[ix + 1] == 0)    {}  // OK - quote as last char is allowed
      else
      {
        return true;  // NOT OK - unauthorized quote found
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* forbiddenMqttTopic -
*/
bool forbiddenMqttTopic(const char* s)
{
  if (s == (void*) 0)
  {
    return false;
  }

  while (*s != 0)
  {
    switch (*s)
    {
    case '+':
    case '#':
      return true;
    }

    ++s;
  }

  return false;
}
