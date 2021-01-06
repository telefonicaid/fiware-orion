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
#include <stdio.h>                                             // snprintf
#include <postgresql/libpq-fe.h>                               // PGconn

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRender.h"                                    // kjRender - TMP
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/pgGeoPolygonPush.h"                     // Own interface



extern bool kjGeoPointExtract(KjNode* coordinatesP, double* longitudeP, double* latitudeP, double* altitudeP);
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

  // <DEBUG>
  char buf[1024];
  kjRender(orionldState.kjsonP, coordinatesP, buf, sizeof(buf));
  LM_TMP(("Coordinates: %s", buf));
  // </DEBUG>

  polygonCoordsString[0] = '(';

  while (subPolygonP != NULL)  // First array
  {
    KjNode* pointP = subPolygonP->value.firstChildP;

    while (pointP != NULL)  // Second Array
    {
      double  longitude;
      double  latitude;
      double  altitude;
      char    pointBuffer[64];

      // <DEBUG>
      char buf[1024];
      kjRender(orionldState.kjsonP, coordinatesP, buf, sizeof(buf));
      LM_TMP(("Coordinates: %s", buf));
      kjRender(orionldState.kjsonP, pointP, buf, sizeof(buf));
      LM_TMP(("Point: %s", buf));
      // </DEBUG>

      if (kjGeoPointExtract(pointP, &longitude, &latitude, &altitude) == false)
        LM_RE(false, ("Internal Error (unable to extract longitude/latitude/altitude for a Point) "));

      if (altitude != 0)
        snprintf(pointBuffer, sizeof(pointBuffer), "%f %f %f", longitude, latitude, altitude);
      else
        snprintf(pointBuffer, sizeof(pointBuffer), "%f %f", longitude, latitude);

      int pointBufferLen = strlen(pointBuffer);

      LM_TMP(("polygonCoordsIx  == %d", polygonCoordsIx));
      LM_TMP(("pointBufferLen   == %d", pointBufferLen));
      LM_TMP(("polygonCoordsLen == %d", polygonCoordsLen));
      LM_TMP(("polygonCoordsString: '%s'", polygonCoordsString));
      if (polygonCoordsIx + pointBufferLen + 1 >= polygonCoordsLen)
        LM_RE(false, ("Not enough room in polygonCoordsString - fix and recompile"));

      if (polygonCoordsIx != 1)  // Add a comma before the (Point), unless it's the first point
      {
        polygonCoordsString[polygonCoordsIx] = ',';
        ++polygonCoordsIx;
      }

      LM_TMP(("Appending '%s' to polygonCoordsString", pointBuffer));
      strncpy(&polygonCoordsString[polygonCoordsIx], pointBuffer, polygonCoordsLen - polygonCoordsIx);
      polygonCoordsIx += pointBufferLen;

      pointP = pointP->next;
    }

    subPolygonP = subPolygonP->next;
  }

  polygonCoordsString[polygonCoordsIx] = ')';

  LM_TMP(("FINAL polygonCoordsString: '%s'", polygonCoordsString));

  return true;
}



// -----------------------------------------------------------------------------
//
// pgGeoPolygonPush - push a Geo-Polygon property to its DB table
//
bool pgGeoPolygonPush
(
  PGconn*      connectionP,
  const char*  opMode,
  KjNode*      coordinatesP,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
  const char*  createdAt,
  const char*  modifiedAt,
  bool         subProperties
)
{
  char*  polygonCoordsString = kaAlloc(&orionldState.kalloc, 10240);

  if (polygonCoordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  if (kjGeoPolygonExtract(coordinatesP, polygonCoordsString, 10240) == false)
    LM_RE(false, ("unable to extract geo-coordinates from Kj-Tree"));

  char*        sql = kaAlloc(&orionldState.kalloc, 12008);
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";

  if (sql == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, '%s', ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, polygonCoordsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, polygonCoordsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, '%s', ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, polygonCoordsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, polygonCoordsString);
  }

  LM_TMP(("SQL[%p]: %s", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
