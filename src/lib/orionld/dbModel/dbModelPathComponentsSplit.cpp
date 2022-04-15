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
#include <string.h>                                              // strcmp

#include "orionld/dbModel/dbModelPathComponentsSplit.h"          // Own interface



// -----------------------------------------------------------------------------
//
// dbModelPathComponentsSplit -
//
int dbModelPathComponentsSplit(char* path, char** compV)
{
  int compIx = 1;

  compV[0] = path;

  while (*path != 0)
  {
    if (*path == '.')
    {
      *path = 0;
      ++path;
      compV[compIx] = path;

      //
      // We only split the first 6 components
      // Max PATH is "attrs.P1.md.Sub-P1.value[.X]*
      //
      if (compIx == 5)
        return 6;

      //
      // We break if we find "attrs.X.value", but not until we have 4 components; "attrs.P1.value.[.X]*"
      // It is perfectly possible 'path' is only "attrs.P1.value", and if so, we'd have left the function already
      //
      if ((compIx == 3) && (strcmp(compV[2], "value") == 0))
        return 4;

      ++compIx;
    }

    ++path;
  }

  return compIx;
}
