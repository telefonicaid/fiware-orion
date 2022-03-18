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
#include <string.h>                                            // strncpy

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoPointExtract.h"                    // kjGeoPointExtract
#include "orionld/troe/kjGeoPolygonExtract.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// kjGeoPolygonExtract -
//
// A POLYGON looks like this:
//
// "value": {
//   "type": "Polygon",
//   "coordinates": [[ [0,0], [4,0], [4,4], [0,4], [0,0] ], [ [1,1], [2,1], [2,2], [1, 2], [1,1] ]]
// }
//
// In PostGIS, the POLYGON would look like this:
//   POLYGON((0 0,  4 0,  4 4,  0 4,  0 0))
//
// A DONUT would look like this:
//   POLYGON((0 0,  4 0,  4 4,  0 4,  0 0),(1 1,  2 1,  2 2,  1 2,  1 1))
//
bool kjGeoPolygonExtract(KjNode* coordinatesP, char* polygonCoordsString, int polygonCoordsLen)
{
  KjNode* subPolygonP     = coordinatesP->value.firstChildP;
  int     polygonCoordsIx = 1;

  // coordinatesP is the toplevel array

  polygonCoordsString[0] = '(';

  while (subPolygonP != NULL)  // First array
  {
    KjNode* pointP = subPolygonP->value.firstChildP;

    while (pointP != NULL)  // Second Array
    {
      double  longitude;
      double  latitude;
      double  altitude = 0;
      char    pointBuffer[64];

      if (kjGeoPointExtract(pointP, &longitude, &latitude, &altitude) == false)
        LM_RE(false, ("Internal Error (unable to extract longitude/latitude/altitude for a Point) "));

      snprintf(pointBuffer, sizeof(pointBuffer), "%f %f %f", longitude, latitude, altitude);

      int pointBufferLen = strlen(pointBuffer);

      if (polygonCoordsIx + pointBufferLen + 1 >= polygonCoordsLen)
        LM_RE(false, ("Not enough room in polygonCoordsString - fix and recompile"));

      if (polygonCoordsIx != 1)  // Add a comma before the (Point), unless it's the first point
      {
        polygonCoordsString[polygonCoordsIx] = ',';
        ++polygonCoordsIx;
      }

      strncpy(&polygonCoordsString[polygonCoordsIx], pointBuffer, polygonCoordsLen - polygonCoordsIx);
      polygonCoordsIx += pointBufferLen;

      pointP = pointP->next;
    }

    subPolygonP = subPolygonP->next;
  }

  polygonCoordsString[polygonCoordsIx] = ')';

  return true;
}
