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
#include "orionld/troe/kjGeoLineStringExtract.h"               // kjGeoLineStringExtract
#include "orionld/troe/pgGeoLineStringPush.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// pgGeoLineStringPush - push a Geo-LineString property to its DB table
//
bool pgGeoLineStringPush
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

  if (observedAt != NULL)
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, ts, instanceId, id, entityId, observedAt, valueType, subProperties, datasetId, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, '%s', ST_GeomFromText('LINESTRING(%s)', 4326))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, observedAt, subPropertiesString, datasetId, lineStringCoordsString);
  }
  else
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, ts, instanceId, id, entityId, valueType, subProperties, datasetId, geoLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'GeoLineString', %s, '%s', ST_GeomFromText('LINESTRING(%s)', 4326))",
             opMode, orionldState.requestTimeString, attributeInstance, attributeName, entityId, subPropertiesString, datasetId, lineStringCoordsString);
  }

  // LM_TMP(("SQL[%p]: %s;", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
