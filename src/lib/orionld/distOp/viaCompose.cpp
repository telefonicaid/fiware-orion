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
#include <stdio.h>                                               // snprintf
#include <string.h>                                              // strlen

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/viaCompose.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// viaCompose -
//
char* viaCompose(char* via, char* self)
{
  int viaLen = 6;  // strlen("Via: ") + '\0'

  if (via != NULL)
    viaLen += strlen(via);
  viaLen += 6 + strlen(self);  // 4: ", 1.1 "

  char* viaHeader = kaAlloc(&orionldState.kalloc, viaLen);

  if (viaHeader == NULL)
    LM_X(1, ("Out of memory (kaAlloc failed to allocate %d bytes", viaLen));

  if (via != NULL)
    snprintf(viaHeader, viaLen, "Via: %s, 1.1 %s", via, self);  // Hardcoding HTTP version 1.1 ...
  else
    snprintf(viaHeader, viaLen, "Via: 1.1 %s", self);

  LM_T(LmtHeaders, ("via: '%s'", viaHeader));
  return viaHeader;
}
