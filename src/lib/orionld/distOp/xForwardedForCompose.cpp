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
#include <stdio.h>                                               // snprintf
#include <string.h>                                              // strlen

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/xForwardedForCompose.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// xForwardedForCompose -
//
char* xForwardedForCompose(char* xForwardedFor, char* newHost)
{
  int xffLen = 18;  // strlen("X-Forwarded-For: ") + '\0'

  if (xForwardedFor != NULL)
    xffLen += strlen(xForwardedFor);
  xffLen += 2 + strlen(newHost);  // 2: ", "

  char* xff = kaAlloc(&orionldState.kalloc, xffLen);

  if (xff == NULL)
    return xForwardedFor;  // If allocation error ... at least keep what you had already ...

  if (xForwardedFor != NULL)
    snprintf(xff, xffLen, "X-Forwarded-For: %s, %s", xForwardedFor, newHost);
  else
    snprintf(xff, xffLen, "X-Forwarded-For: %s", newHost);

  LM_T(LmtHeaders, ("xForwardedFor: '%s'", xff));
  return xff;
}
