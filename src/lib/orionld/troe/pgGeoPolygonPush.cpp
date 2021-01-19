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
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoPolygonExtract.h"                  // kjGeoPolygonExtract
#include "orionld/troe/pgGeoPolygonPush.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// pgGeoPolygonPush - push a Geo-Polygon property to its DB table
//
bool pgGeoPolygonPush
(
  PGconn*      connectionP,
  const char*  opMode,
  KjNode*      coordinatesP,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
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
             "opMode, ts, instanceId, id, entityId, observedAt, valueType, subProperties, datasetId, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, '%s', ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, observedAt, subPropertiesString, datasetId, polygonCoordsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, ts, instanceId, id, entityId, valueType, subProperties, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, subPropertiesString, polygonCoordsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, ts, instanceId, id, entityId, valueType, subProperties, datasetId, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, '%s', ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, subPropertiesString, datasetId, polygonCoordsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, ts, instanceId, id, entityId, observedAt, valueType, subProperties, geoPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'GeoPolygon', %s, ST_GeomFromText('POLYGON(%s)', 4267))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, observedAt, subPropertiesString, polygonCoordsString);
  }

  LM_TMP(("SQL[%p]: %s;", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
