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
#include <string.h>                                              // strstr



// -----------------------------------------------------------------------------
//
// xForwardedForMatch -
//
bool xForwardedForMatch(char* hostsHeader, char* host)
{
  //
  // For example:
  //   hostsHeader == "X-Forwarded-For: host1:1026, host2:1028"
  //   host        == "host2:1028"
  //
  // It's a match if (all three match):
  //   1. "host" is found as a subsctring of hostsHeader
  //   2. the char before is either ':', ',', or ' '   AND
  //   3. The char after is either '\0', ' ', or ','
  //
  if (hostsHeader == NULL)
    return false;

  char* subString = strstr(hostsHeader, host);
  if (subString == NULL)
    return false;

  char charBefore = subString[-1];
  char charAFter  = subString[strlen(host)];

  if ((charBefore != ' ') && (charBefore != ':') && (charBefore != ','))
    return false;

  if ((charAFter != 0) && (charAFter != ',') && (charAFter != ' '))
    return false;

  return true;
}
