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
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/pgGeoSubPointPush.h"                    // pgGeoSubPointPush
#include "orionld/troe/pgGeoSubPolygonPush.h"                  // pgGeoSubPolygonPush
#include "orionld/troe/pgGeoSubLineStringPush.h"               // pgGeoSubLineStringPush
#include "orionld/troe/pgGeoSubMultiPolygonPush.h"             // pgGeoSubMultiPolygonPush
#include "orionld/troe/pgGeoSubMultiLineStringPush.h"          // pgGeoSubMultiLineStringPush
#include "orionld/troe/pgGeoSubPropertyPush.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// pgGeoPropertyPush - push a Geo SubProperty to its DB table
//
bool pgGeoSubPropertyPush
(
  PGconn*      connectionP,
  KjNode*      valueNodeP,
  const char*  instanceId,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeRef,
  const char*  attributeId,
  const char*  subAttributeName,
  const char*  observedAt
)
{
  KjNode*  geoTypeP     = kjLookup(valueNodeP, "type");
  KjNode*  coordinatesP = kjLookup(valueNodeP, "coordinates");
  bool     ok;

  if (geoTypeP            == NULL)     LM_RE(false, ("Bad Input (geometry type missing)"));
  if (coordinatesP        == NULL)     LM_RE(false, ("Bad Input (coordinates missing)"));
  if (geoTypeP->type     != KjString)  LM_RE(false, ("Bad Input (geometry type must be a string)"));
  if (coordinatesP->type != KjArray)   LM_RE(false, ("Bad Input (coordinates must be an array)"));


  if (strcmp(geoTypeP->value.s, "Point") == 0)
    ok = pgGeoSubPointPush(connectionP, coordinatesP, instanceId, entityRef, entityId, attributeRef, attributeId, subAttributeName, observedAt);
  else if (strcmp(geoTypeP->value.s, "Polygon") == 0)
    ok = pgGeoSubPolygonPush(connectionP, coordinatesP, instanceId, entityRef, entityId, attributeRef, attributeId, subAttributeName, observedAt);
  else if (strcmp(geoTypeP->value.s, "LineString") == 0)
    ok = pgGeoSubLineStringPush(connectionP, coordinatesP, instanceId, entityRef, entityId, attributeRef, attributeId, subAttributeName, observedAt);
  else if (strcmp(geoTypeP->value.s, "MultiPolygon") == 0)
    ok = pgGeoSubMultiPolygonPush(connectionP, coordinatesP, instanceId, entityRef, entityId, attributeRef, attributeId, subAttributeName, observedAt);
  else if (strcmp(geoTypeP->value.s, "MultiLineString") == 0)
    ok = pgGeoSubMultiLineStringPush(connectionP, coordinatesP, instanceId, entityRef, entityId, attributeRef, attributeId, subAttributeName, observedAt);
  else
    LM_RE(false, ("Bad Input (invalid geometry for GeoProperty: %s)", geoTypeP->value.s));

  return ok;
}
