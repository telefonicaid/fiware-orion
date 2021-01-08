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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/kjGeoPointExtract.h"                    // kjGeoPointExtract
#include "orionld/troe/pgGeoPointPush.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// pgGeoPointPush - push a Geo-Point property to its DB table
//
bool pgGeoPointPush
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
  double longitude;
  double latitude;
  double altitude;

  if (kjGeoPointExtract(coordinatesP, &longitude, &latitude, &altitude) == false)
    LM_RE(false, ("unable to extract geo-coordinates from Kj-Tree"));

  char         sql[1024];
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPoint', %s, '%s', ST_GeomFromText('POINT Z(%f %f %f)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, longitude, latitude, altitude);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPoint', %s, ST_GeomFromText('POINT Z(%f %f %f)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, longitude, latitude, altitude);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPoint', %s, '%s', ST_GeomFromText('POINT Z(%f %f %f)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, longitude, latitude, altitude);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoPoint', %s, ST_GeomFromText('POINT Z(%f %f %f)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, longitude, latitude, altitude);
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
