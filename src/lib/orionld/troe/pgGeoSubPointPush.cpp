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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoPointExtract.h"                    // kjGeoPointExtract
#include "orionld/troe/pgGeoSubPointPush.h"                    // Own interface


// -----------------------------------------------------------------------------
//
// pgGeoSubPointPush -
//
bool pgGeoSubPointPush
(
  PGconn*      connectionP,
  KjNode*      coordinatesP,
  const char*  instanceId,
  const char*  entityId,
  const char*  attributeId,
  const char*  subAttributeName,
  const char*  observedAt
)
{
  double longitude;
  double latitude;
  double altitude;

  if (kjGeoPointExtract(coordinatesP, &longitude, &latitude, &altitude) == false)
    LM_RE(false, ("unable to extract geo-coordinates from Kj-Tree"));

  char         sql[1024];
  PGresult*    res;

  //
  // Two combinations for NULL/non-NULL 'observedAt'
  //
  if (observedAt != NULL)
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, ts, id, entityId, attributeId, observedAt, valueType, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'GeoPoint', ST_GeomFromText('POINT Z(%f %f %f)'))",
             instanceId, orionldState.requestTimeString, subAttributeName, entityId, attributeId, observedAt, longitude, latitude, altitude);
  }
  else
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, ts, id, entityId, attributeId, valueType, geoPoint) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'GeoPoint', ST_GeomFromText('POINT Z(%f %f %f)'))",
             instanceId, orionldState.requestTimeString, subAttributeName, entityId, attributeId, longitude, latitude, altitude);
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
