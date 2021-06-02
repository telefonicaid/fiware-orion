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
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgQuotedString.h"                       // pgQuotedString
#include "orionld/troe/kjGeoPointExtract.h"                    // kjGeoPointExtract
#include "orionld/troe/kjGeoMultiPointExtract.h"               // kjGeoMultiPointExtract
#include "orionld/troe/kjGeoLineStringExtract.h"               // kjGeoLineStringExtract
#include "orionld/troe/kjGeoMultiLineStringExtract.h"          // kjGeoMultiLineStringExtract
#include "orionld/troe/kjGeoPolygonExtract.h"                  // kjGeoPolygonExtract
#include "orionld/troe/kjGeoMultiPolygonExtract.h"             // kjGeoMultiPolygonExtract
#include "orionld/troe/pgAttributeAppend.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pgAttributeAppend -
//
// INSERT INTO attributes(instanceId,
//                        id,
//                        opMode,
//                        entityId,
//                        observedAt,
//                        subProperties,
//                        unitCode,
//                        datasetId,
//                        valueType,
//
//                        text,
//                        boolean,
//                        number,
//                        datetime,
//                        compound,
//                        geoPoint,
//                        geoPolygon,
//                        geoMultiPolygon,
//                        geoLineString,
//                        geoMultiLineString,
//                        ts) VALUES   ('', '', '', ...), ('', '', '', ...), ('', '', '', ...), ...
//
// This function appends a new ('', '', '', ...) for the VALUES of attributesBuffer
// It also calls pgSubAttributeBuild for any sub-attributes
//
void pgAttributeAppend
(
  PgAppendBuffer*  attributesBufferP,
  const char*      instanceId,
  const char*      attributeName,
  const char*      opMode,
  const char*      entityId,
  char*            type,
  char*            observedAt,    // Can be NULL
  bool             subProperties,
  char*            unitCode,      // Can be NULL
  char*            datasetId,     // Can be NULL
  KjNode*          valueNodeP,
  char*            object
)
{
  int         bufSize = 32 * 1024;
  char*       buf     = kaAlloc(&orionldState.kalloc, bufSize);
  const char* comma   = (attributesBufferP->values != 0)? "," : "";

  if (buf == NULL)
    LM_X(1, ("out of memory allocating %d bytes", bufSize));

  observedAt = (observedAt == NULL)? (char*) "null" : pgQuotedString(observedAt);
  unitCode   = (unitCode   == NULL)? (char*) "null" : pgQuotedString(unitCode);

  if (datasetId == NULL)
    datasetId  = (char*) "None";

  const char* hasSubProperties = (subProperties == true)? "true" : "false";

  if (strcmp(opMode, "Delete") == 0)
  {
    snprintf(buf, bufSize, "%s('%s', '%s', 'Delete', '%s', null, null, null, '%s', null, null, null, null, null, null, null, null, null, null, null, null, '%s')",
             comma, instanceId, attributeName, entityId, datasetId, orionldState.requestTimeString);
  }
  else if (strcmp(type, "Relationship") == 0)
  {
    snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Relationship', '%s', null, null, null, null, null, null, null, null, null, null, '%s')",
             comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, object, orionldState.requestTimeString);
  }
  else if (strcmp(type, "GeoProperty") == 0)
  {
    KjNode*      geoTypeNodeP     = kjLookup(valueNodeP, "type");
    KjNode*      coordinatesNodeP = kjLookup(valueNodeP, "coordinates");
    const char*  geoType          = geoTypeNodeP->value.s;

    if (strcmp(geoType, "Point") == 0)
    {
      double longitude;
      double latitude;
      double altitude;

      kjGeoPointExtract(coordinatesNodeP, &longitude, &latitude, &altitude);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoPoint', null, null, null, null, null, ST_GeomFromText('POINT(%f %f %f)'), null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, longitude, latitude, altitude, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "MultiPoint") == 0)
    {
      char*  coordsString = kaAlloc(&orionldState.kalloc, 2048);

      kjGeoMultiPointExtract(coordinatesNodeP, coordsString, 2048);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoMultiPoint', null, null, null, null, null, null, ST_GeomFromText('MULTIPOINT(%s)', 4326), null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "LineString") == 0)
    {
      char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

      kjGeoLineStringExtract(coordinatesNodeP, coordsString, 10240);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoLineString', null, null, null, null, null, null, null, null, null, ST_GeomFromText('LINESTRING(%s)', 4326), null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "MultiLineString") == 0)
    {
      char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

      kjGeoMultiLineStringExtract(coordinatesNodeP, coordsString, 10240);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoMultiLineString', null, null, null, null, null, null, null, null, null, null, ST_GeomFromText('MULTILINESTRING(%s)', 4326), '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "Polygon") == 0)
    {
      char* coordsString = kaAlloc(&orionldState.kalloc, 2048);

      kjGeoPolygonExtract(coordinatesNodeP, coordsString, 2048);
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoPolygon', null, null, null, null, null, null, null, ST_GeomFromText('POLYGON(%s)'), null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "MultiPolygon") == 0)
    {
      char* coordsString = kaAlloc(&orionldState.kalloc, 4096);

      kjGeoMultiPolygonExtract(coordinatesNodeP, coordsString, 4096);
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'GeoMultiPolygon', null, null, null, null, null, null, null, null, ST_GeomFromText('MULTIPOLYGON(%s)', 4326), null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, coordsString, orionldState.requestTimeString);
    }
  }
  else  // Property
  {
    if (valueNodeP->type == KjString)
    {
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'String', '%s', null, null, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.s, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjBoolean)
    {
      const char* value = (valueNodeP->value.b == true)? "true" : "false";

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Boolean', null, %s, null, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, value, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjInt)
    {
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Number', null, null, %lld, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.i, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjFloat)
    {
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Number', null, null, %f, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.f, orionldState.requestTimeString);
    }
    else if ((valueNodeP->type == KjArray) || (valueNodeP->type == KjObject))
    {
      // WARNING: If an attribute is HUGE, it may not have room enough in a buffer allocated by kaAlloc (there's a max-size)
      int          renderedValueSize   = kjFastRenderSize(valueNodeP);
      char*        renderedValue       = kaAlloc(&orionldState.kalloc, renderedValueSize);

      kjFastRender(valueNodeP, renderedValue);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Compound', null, null, null, null, '%s', null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, renderedValue, orionldState.requestTimeString);
    }
  }

  if (buf[0] == 0)
  {
    LM_W(("TROE: To Be Implemented"));
    return;
  }

  pgAppend(attributesBufferP, buf, 0);
  attributesBufferP->values += 1;
}
