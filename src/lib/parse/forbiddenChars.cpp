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
bool forbiddenIdChars(const std::string& api, const char* s, const char* exceptions)
{
  if (api == "v1" && !checkIdv1)
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
