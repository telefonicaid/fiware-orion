/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoLineStringExtract.h"               // kjGeoLineStringExtract
#include "orionld/troe/kjGeoMultiLineStringExtract.h"          // Own interface



// -----------------------------------------------------------------------------
//
// kjGeoLineStringExtract -
//
bool kjGeoMultiLineStringExtract(KjNode* coordinatesP, char* coordsString, int coordsLen)
{
  char*   lineStringCoords = kaAlloc(&orionldState.kalloc, 1024);
  int     coordsIx         = 0;

  if (lineStringCoords == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  for (KjNode* lineStringP = coordinatesP->value.firstChildP; lineStringP != NULL; lineStringP = lineStringP->next)
  {
    if (kjGeoLineStringExtract(lineStringP, lineStringCoords, 1024) == false)
      LM_RE(false, ("kjGeoLineStringExtract failed"));

    int slen = strlen(lineStringCoords);
    if (coordsIx + slen + 3 >= coordsLen)
      LM_RE(false, ("Internal Error (not enough room in coordsString)"));

    if (coordsIx != 0)
    {
      coordsString[coordsIx] = ',';
      ++coordsIx;
    }

    coordsString[coordsIx] = '(';
    ++coordsIx;

    strncpy(&coordsString[coordsIx], lineStringCoords, coordsLen - coordsIx);
    coordsIx += slen;

    coordsString[coordsIx] = ')';
    ++coordsIx;
  }

  return true;
}
