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



extern bool kjGeoLineStringExtract(KjNode* coordinatesP, char* lineStringCoordsString, int lineStringCoordsLen);
// -----------------------------------------------------------------------------
//
// kjGeoMultiLineStringExtract -
//
// MULTILINESTRING((0.000000 0.000000,1.000000 1.000000,4.000000 4.000000,1.000000 1.000000,2.000000 2.000000,5.000000 5.000000))'
// MULTILINESTRING(((0.000000 0.000000,1.000000 1.000000,4.000000 4.000000), (1.000000 1.000000,2.000000 2.000000,5.000000 5.000000)))'
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

  LM_TMP(("FINAL coordsString: %s", coordsString));
  return true;
}



// -----------------------------------------------------------------------------
//
// pgGeoMultiLineStringPush - push a Geo-MultiLineString property to its DB table
//
bool pgGeoMultiLineStringPush
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
  char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

  if (coordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  if (kjGeoMultiLineStringExtract(coordinatesP, coordsString, 10240) == false)
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
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', %s, '%s', ST_GeomFromText('MULTILINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, coordsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', %s, ST_GeomFromText('MULTILINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, coordsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', %s, '%s', ST_GeomFromText('MULTILINESTRING(%s)'))",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, coordsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, 12007, "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', %s, ST_GeomFromText('MULTILINESTRING(%s)'))",
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
