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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoPolygonExtract.h"                  // kjGeoPolygonExtract
#include "orionld/troe/kjGeoMultiPolygonExtract.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjGeoMultiPolygonExtract -
//
bool kjGeoMultiPolygonExtract(KjNode* coordinatesP, char* coordsString, int coordsLen)
{
  char* polygonCoordsString = kaAlloc(&orionldState.kalloc, 2048);
  int   coordsIx            = 1;

  if (polygonCoordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  coordsString[0] = '(';

  for (KjNode* polygonP = coordinatesP->value.firstChildP; polygonP != NULL; polygonP = polygonP->next)
  {
    if (kjGeoPolygonExtract(polygonP, polygonCoordsString, 2048) == false)
      LM_RE(false, ("kjGeoPolygonExtract failed"));

    int slen = strlen(polygonCoordsString);
    if (coordsIx + slen + 1 >= coordsLen)
      LM_RE(false, ("Internal Error (not enough room in coordsString)"));

    if (coordsIx != 1)
    {
      coordsString[coordsIx] = ',';
      ++coordsIx;
    }

    LM_TMP(("polygonCoordsString: '%s'", polygonCoordsString));
    LM_TMP(("Room left: %d bytes", coordsLen - coordsIx));
    strncpy(&coordsString[coordsIx], polygonCoordsString, coordsLen - coordsIx);
    coordsIx += slen;
    LM_TMP(("coordsString: %s", coordsString));
  }
  coordsString[coordsIx] = ')';

  LM_TMP(("FINAL coordsString: %s", coordsString));
  return true;
}
