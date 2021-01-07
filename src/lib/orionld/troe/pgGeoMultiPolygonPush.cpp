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
#include <postgresql/libpq-fe.h>                               // PGconn

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/pgGeoMultiPolygonPush.h"                // Own interface



extern bool kjGeoPolygonExtract(KjNode* coordinatesP, char* polygonCoordsString, int polygonCoordsLen);
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



// -----------------------------------------------------------------------------
//
// pgGeoMultiPolygonPush - push a Geo-MultiPolygon property to its DB table
//
bool pgGeoMultiPolygonPush
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
  char* coordsString = kaAlloc(&orionldState.kalloc, 8 * 1024);

  if (coordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  if (kjGeoMultiPolygonExtract(coordinatesP, coordsString, 8 * 1024) == false)
    LM_RE(false, ("unable to extract geo-coordinates from Kj-Tree"));

  int          sqlSize = 10 * 1024;
  char*        sql     = kaAlloc(&orionldState.kalloc, sqlSize + 1);
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";

  if (sql == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, geoMultiPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiPolygon', %s, '%s', ST_GeomFromText('MULTIPOLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, coordsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, geoMultiPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiPolygon', %s, ST_GeomFromText('MULTIPOLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, coordsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, geoMultiPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiPolygon', %s, '%s', ST_GeomFromText('MULTIPOLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, coordsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, geoMultiPolygon) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiPolygon', %s, ST_GeomFromText('MULTIPOLYGON(%s)', 4267))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, coordsString);
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
