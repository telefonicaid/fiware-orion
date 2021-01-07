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
#include "orionld/troe/pgGeoLineStringPush.h"                  // Own interface



extern bool kjGeoPointExtract(KjNode* coordinatesP, double* longitudeP, double* latitudeP, double* altitudeP);
// -----------------------------------------------------------------------------
//
// kjGeoLineStringExtract -
//
// A LINESTRING looks like this:
//
// "value": {
//   "type": "LineString",
//   "coordinates": [ [0,0], [4,0], [4,4], [0,4] ]
// }
//
// In PostGIS, the LINESTRING would look like this:
//   LINESTRING(0 0,  4 0,  4 4,  0 4,  0 0)
//
bool kjGeoLineStringExtract(KjNode* coordinatesP, char* lineStringCoordsString, int lineStringCoordsLen)
{
  int     lineStringCoordsIx = 0;
  KjNode* pointP = coordinatesP->value.firstChildP;


  while (pointP != NULL)  // Second Array
  {
    double  longitude;
    double  latitude;
    double  altitude;
    char    pointBuffer[64];

    if (kjGeoPointExtract(pointP, &longitude, &latitude, &altitude) == false)
      LM_RE(false, ("Internal Error (unable to extract longitude/latitude/altitude for a Point) "));

    if (altitude != 0)
      snprintf(pointBuffer, sizeof(pointBuffer), "%f %f %f", longitude, latitude, altitude);
    else
      snprintf(pointBuffer, sizeof(pointBuffer), "%f %f", longitude, latitude);

    int pointBufferLen = strlen(pointBuffer);

    if (lineStringCoordsIx + pointBufferLen + 1 >= lineStringCoordsLen)
      LM_RE(false, ("Not enough room in lineStringCoordsString - fix and recompile"));

    if (lineStringCoordsIx != 0)  // Add a comma before the Point, unless it's the first point
    {
      lineStringCoordsString[lineStringCoordsIx] = ',';
      ++lineStringCoordsIx;
    }

    LM_TMP(("Appending '%s' to lineStringCoordsString", pointBuffer));
    strncpy(&lineStringCoordsString[lineStringCoordsIx], pointBuffer, lineStringCoordsLen - lineStringCoordsIx);
    lineStringCoordsIx += pointBufferLen;

    pointP = pointP->next;
  }

  LM_TMP(("FINAL lineStringCoordsString: '%s'", lineStringCoordsString));

  return true;
}



// -----------------------------------------------------------------------------
//
// pgGeoLineStringPush - push a Geo-LineString property to its DB table
//
bool pgGeoLineStringPush
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
  char*  lineStringCoordsString = kaAlloc(&orionldState.kalloc, 10240);

  if (lineStringCoordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  if (kjGeoLineStringExtract(coordinatesP, lineStringCoordsString, 10240) == false)
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
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, '%s', ST_GeomFromText('LINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, lineStringCoordsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, ST_GeomFromText('LINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, lineStringCoordsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, '%s', ST_GeomFromText('LINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, lineStringCoordsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, ST_GeomFromText('LINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, lineStringCoordsString);
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
