/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include <string.h>                                              // strstr

#include "logMsg/logMsg.h"                                       // LM_*



// -----------------------------------------------------------------------------
//
// viaMatch -
//
bool viaMatch(char* currentVia, char* next)
{
  LM_T(LmtDistOpLoop, ("Loop Detection: Via header:        '%s' (from 'forwarder')", currentVia));
  LM_T(LmtDistOpLoop, ("Loop Detection: Forward to source: '%s' (next 'forwardee')", next));

  //
  // For example:
  //   currentVia == "Via: 1.1 CB, 1.1 CP1"
  //   next       == "CP1"
  //
  // It's a match if (all three match):
  //   1. "next" is found as a substring of currentVia
  //   2. the char before is ' '   AND
  //   3. The char after is either '\0', ' ', or ','
  //
  if (currentVia == NULL)
  {
    LM_T(LmtDistOpLoop, ("Loop Detection: No Via header present, so no loop detected"));
    return false;
  }

  char* subString = strstr(currentVia, next);
  if (subString == NULL)
  {
    LM_T(LmtDistOpLoop, ("Loop Detection: IP/port (%s) not in Via header, so no loop detected", next));
    return false;
  }

  char charBefore = (subString == currentVia)? ' ' : subString[-1];
  char charAfter  = subString[strlen(next)];

  LM_T(LmtDistOpLoop, ("Loop Detection: subString: '%s'", subString));
  LM_T(LmtDistOpLoop, ("Loop Detection: charBefore=0x%x, charAfter=0x%x", charBefore & 0xFF, charAfter & 0xFF));
  if (charBefore != ' ')
  {
    LM_T(LmtDistOpLoop, ("Loop Detection: No Match"));
    return false;
  }

  if ((charAfter != 0) && (charAfter != ',') && (charAfter != ' '))
  {
    LM_T(LmtDistOpLoop, ("Loop Detection: No Match"));
    return false;
  }

  LM_T(LmtDistOpLoop, ("Loop Detection: Detected a loop - must stop it!"));
  return true;
}
