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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/dbModel/dbModelValueStrip.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// dbModelValueStrip -
//
void dbModelValueStrip(KjNode* nodeP)
{
  while (true)
  {
    char* dotValue = strstr(nodeP->value.s, ".value");

    if (dotValue == NULL)
      break;

    // Left-Shift six steps
    char* to = dotValue;

    while (to[6] != 0)
    {
      to[0] = to[6];
      ++to;
    }
    *to = 0;
  }
}
