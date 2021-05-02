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
#include "kjson/kjBuilder.h"                                   // kjChildRemove, kjChildAdd, ...
#include "kjson/kjRender.h"                                    // kjFastRender
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/troe.h"                                 // TroeMode, troeMode
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgQuotedString.h"                       // pgQuotedString
#include "orionld/troe/pgObservedAtExtract.h"                  // pgObservedAtExtract
#include "orionld/troe/kjGeoPointExtract.h"                    // kjGeoPointExtract
#include "orionld/troe/kjGeoLineStringExtract.h"               // kjGeoLineStringExtract
#include "orionld/troe/kjGeoMultiLineStringExtract.h"          // kjGeoMultiLineStringExtract
#include "orionld/troe/kjGeoPolygonExtract.h"                  // kjGeoPolygonExtract
#include "orionld/troe/kjGeoMultiPolygonExtract.h"             // kjGeoMultiPolygonExtract
#include "orionld/troe/pgSubAttributeBuild.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// attributeAppend -
//
// INSERT INTO subAttributes(instanceId,
//                           id,
//                           entityId,
//                           attrInstanceId,
//                           attrDatasetId,
//                           observedAt,
//                           unitCode,
//                           valueType,
//
//                           text,
//                           boolean,
//                           number,
//                           datetime,
//                           compound,
//                           geoPoint,
//                           geoPolygon,
//                           geoMultiPolygon,
//                           geoLineString,
//                           geoMultiLineString,
//                           ts) VALUES   ('', '', '', ...), ('', '', '', ...), ('', '', '', ...), ...
//
// This function appends a new ('', '', '', ...) for the VALUES of subAttributesBuffer
//
static void subAttributeAppend
(
  PgAppendBuffer*  subAttributesBufferP,
  const char*      instanceId,
  const char*      subAttributeName,
  const char*      entityId,
  const char*      attrInstanceId,
  char*            attrDatasetId,  // might be NULL, but can't be in the DB
  const char*      type,
  char*            observedAt,     // Can be NULL
  char*            unitCode,       // Can be NULL
  KjNode*          valueNodeP,
  const char*      object
)
{
  LM_TMP(("TROE2: Treating sub-attr '%s' (valueNodeP at %p)", subAttributeName, valueNodeP));

  int         bufSize = 20480;
  char*       buf     = kaAlloc(&orionldState.kalloc, bufSize);
  const char* comma   = (subAttributesBufferP->values != 0)? "," : "";

  attrDatasetId = (attrDatasetId == NULL)? (char*) "'None'" : pgQuotedString(attrDatasetId);
  observedAt    = (observedAt    == NULL)? (char*) "null"   : pgQuotedString(observedAt);
  unitCode      = (unitCode      == NULL)? (char*) "null"   : pgQuotedString(unitCode);

  if (strcmp(type, "Relationship") == 0)
  {
    LM_TMP(("TROE: Appending 'Relationship' sub-attribute '%s'", subAttributeName));
    snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'Relationship', '%s', null, null, null, null, null, null, null, null, null, '%s')",
             comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, object, orionldState.requestTimeString);
  }
  else if (strcmp(type, "GeoProperty") == 0)
  {
    KjNode*     geoTypeNodeP     = kjLookup(valueNodeP, "type");
    KjNode*     coordinatesNodeP = kjLookup(valueNodeP, "coordinates");
    const char* geoType          = geoTypeNodeP->value.s;

    if (strcmp(geoType, "Point") == 0)
    {
      double longitude;
      double latitude;
      double altitude;

      kjGeoPointExtract(coordinatesNodeP, &longitude, &latitude, &altitude);
      LM_TMP(("TROE: Appending 'Geo::Point' sub-attribute '%s'", subAttributeName));

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'GeoPoint', null, null, null, null, null, ST_GeomFromText('POINT Z(%f %f %f)'), null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, longitude, latitude, altitude, orionldState.requestTimeString);
      LM_TMP(("TROE: point buffer: '%s'", buf));
    }
    else if (strcmp(geoType, "MultiPoint") == 0)
    {
    }
    else if (strcmp(geoType, "LineString") == 0)
    {
      LM_TMP(("TROE: got a line-string"));
      char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

      kjGeoLineStringExtract(coordinatesNodeP, coordsString, 10240);
      LM_TMP(("TROE: Appending 'Geo::LineString' sub-attribute '%s'", subAttributeName));

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'GeoLineString', null, null, null, null, null, null, null, null, ST_GeomFromText('LINESTRING(%s)', 4326), null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, coordsString, orionldState.requestTimeString);
      LM_TMP(("TROE: line-string buffer: '%s'", buf));
    }
    else if (strcmp(geoType, "MultiLineString") == 0)
    {
      char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

      kjGeoMultiLineStringExtract(coordinatesNodeP, coordsString, 10240);
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'GeoMultiLineString', null, null, null, null, null, null, null, null, null, ST_GeomFromText('MULTILINESTRING(%s)', 4326), '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "Polygon") == 0)
    {
      char* coordsString = kaAlloc(&orionldState.kalloc, 2048);

      kjGeoPolygonExtract(coordinatesNodeP, coordsString, 2048);
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'GeoPolygon', null, null, null, null, null, null, ST_GeomFromText('POLYGON(%s)'), null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, coordsString, orionldState.requestTimeString);
    }
    else if (strcmp(geoType, "MultiPolygon") == 0)
    {
      char* coordsString = kaAlloc(&orionldState.kalloc, 4096);

      kjGeoMultiPolygonExtract(coordinatesNodeP, coordsString, 4096);
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'GeoMultiPolygon', null, null, null, null, null, null, null, ST_GeomFromText('MULTIPOLYGON(%s)', 4326), null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, coordsString, orionldState.requestTimeString);
    }
  }
  else  // Property
  {
    if (valueNodeP->type == KjString)
    {
      LM_TMP(("TROE: Appending 'String' sub-attribute '%s'", subAttributeName));
      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'String', '%s', null, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, valueNodeP->value.s, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjBoolean)
    {
      LM_TMP(("TROE: Appending 'Boolean' sub-attribute '%s'", subAttributeName));
      const char* value = (valueNodeP->value.b == true)? "true" : "false";

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'Boolean', null, %s, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, value, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjInt)
    {
      LM_TMP(("TROE: Appending 'Integer' sub-attribute '%s'", subAttributeName));

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'Number', null, null, %lld, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, valueNodeP->value.i, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjFloat)
    {
      LM_TMP(("TROE: Appending 'Float' sub-attribute '%s'", subAttributeName));

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'Number', null, null, %f, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, valueNodeP->value.f, orionldState.requestTimeString);
    }
    else if ((valueNodeP->type == KjArray) || (valueNodeP->type == KjObject))
    {
      LM_TMP(("TROE: Appending 'Compound' sub-attribute '%s'", subAttributeName));
      int          renderedValueSize   = 4 * 1024;
      char*        renderedValue       = kaAlloc(&orionldState.kalloc, renderedValueSize);

      kjFastRender(orionldState.kjsonP, valueNodeP, renderedValue, renderedValueSize);

      snprintf(buf, bufSize, "%s('%s', '%s', '%s', '%s', %s, %s, %s, 'Compound', null, null, null, null, '%s', null, null, null, null, null, '%s')",
               comma, instanceId, subAttributeName, entityId, attrInstanceId, attrDatasetId, observedAt, unitCode, renderedValue, orionldState.requestTimeString);
    }
  }

  pgAppend(subAttributesBufferP, buf, 0);
  LM_TMP(("TROE: sub-attribute buffer: '%s'", subAttributesBufferP->buf));
  subAttributesBufferP->values += 1;
  LM_TMP(("TROE: values in sub-attribute buffer: %d", subAttributesBufferP->values));
}



// ----------------------------------------------------------------------------
//
// pgSubAttributeBuild -
//
bool pgSubAttributeBuild
(
  PgAppendBuffer*  subAttributesBuffer,
  const char*      entityId,
  char*            attrInstanceId,
  char*            attrDatasetId,
  KjNode*          subAttributeNodeP
)
{
  char    instanceId[80];
  char*   type          = NULL;
  char*   observedAt    = NULL;
  char*   unitCode      = NULL;
  char*   object        = NULL;  // Only for relationships
  KjNode* valueNodeP    = NULL;

  LM_TMP(("TROE: Building sub-attribute '%s'", subAttributeNodeP->name));

  if (subAttributeNodeP->type != KjObject)
  {
    LM_TMP(("TROE2: sub-attribute '%s' is not an object (%s) - skipped", subAttributeNodeP->name, kjValueType(subAttributeNodeP->type)));
    return true;
  }

  uuidGenerate(instanceId, sizeof(instanceId), true);

  // Extract sub-attribute info
  for (KjNode* nodeP = subAttributeNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if      (strcmp(nodeP->name, "observedAt") == 0)  observedAt = pgObservedAtExtract(nodeP);
    else if (strcmp(nodeP->name, "unitCode")   == 0)  unitCode   = nodeP->value.s;
    else if (strcmp(nodeP->name, "type")       == 0)  type       = nodeP->value.s;
    else if (strcmp(nodeP->name, "value")      == 0)  valueNodeP = nodeP;
    else if (strcmp(nodeP->name, "object")     == 0)  object     = nodeP->value.s;
    else if (strcmp(nodeP->name, "createdAt")  == 0)  {}  // Skipping
    else if (strcmp(nodeP->name, "modifiedAt") == 0)  {}  // Skipping
    else
    {
      // Sub-sub-attributes are ignored in TRoE
    }
  }

  subAttributeAppend(subAttributesBuffer, instanceId, subAttributeNodeP->name, entityId, attrInstanceId, attrDatasetId, type, observedAt, unitCode, valueNodeP, object);

  return true;
}
