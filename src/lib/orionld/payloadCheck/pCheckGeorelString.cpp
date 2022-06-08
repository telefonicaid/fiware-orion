/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                             // strchr

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kalloc/kaStrdup.h"                                    // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/types/OrionldGeometry.h"                      // OrionldGeometry
#include "orionld/types/OrionldGeoInfo.h"                       // OrionldGeoInfo
#include "orionld/payloadCheck/pCheckGeorel.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeorelString -
//
bool pCheckGeorelString(const char* georel, OrionldGeoInfo* geoInfoP)
{
  //
  // Valid values for georel:
  // * near        - Point only             OK   Point queried to match Points in DB that are close enough
  // * within      - Polygon|MultiPolygon   OK   Polygon used to find DB geometries inside it
  // * contains    - Point only
  // * overlaps    - Polygon Only
  // * intersects  - Polygon Only           OK
  // * equals      - Whatever               OK, I guess
  // * disjoint    - Polygon Only           NOT intersects ?
  //
  // Any other value and it's an error
  //
  //
  if (strncmp(georel, "near;", 5) == 0)
  {
    if (geoInfoP->geometry != GeoPoint)
    {
      orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "Geometry must be 'Point' for georel 'near'", 400);
      return false;
    }
  }
  else if (strcmp(georel, "within") == 0)
  {
    if ((geoInfoP->geometry != GeoPolygon) && (geoInfoP->geometry != GeoMultiPolygon))
    {
      orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "Geometry must be either 'Polygon' or 'MultiPolygon' for georel 'within'", 400);
      return false;
    }
    geoInfoP->georel = GeorelWithin;
  }
  else if (strcmp(georel, "overlaps") == 0)
  {
    // FIXME: Can be any Geometry ... Right?
    geoInfoP->georel = GeorelOverlaps;
  }
  else if (strcmp(georel, "intersects") == 0)
  {
    // FIXME: Should be possible to use any Geomtry ...
    //        But, the mongodb docs talk about Polygon or MultiPolygon:
    //          https://docs.mongodb.com/manual/reference/operator/query/geoIntersects/
    //        For now, I'll allow any Geometry
    geoInfoP->georel = GeorelIntersects;
  }
  else if (strcmp(georel, "disjoint") == 0)
  {
    // Uses $not { 'intersects-expression' }, so, if 'intersects' is only Polygons, then 'disjoint' also
    geoInfoP->georel = GeorelDisjoint;
  }
  else if (strcmp(georel, "contains") == 0)
  {
    geoInfoP->georel = GeorelContains;
  }
  else if (strcmp(georel, "equals") == 0)
  {
    // Valid for all geometry types
    geoInfoP->georel = GeorelEquals;
  }
  else
  {
    orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter - invalid 'georel'", georel, 400);
    return false;
  }

  if (geoInfoP->geometry == GeoPoint)
  {
    //
    // For a Point, georel can have the following values:
    // - near
    //
    char* extra      = NULL;
    char* grel       = kaStrdup(&orionldState.kalloc, georel);
    char* semicolonP;

    if ((semicolonP = strchr(grel, ';')) != NULL)
    {
      *semicolonP = 0;
      extra = &semicolonP[1];

      if (strcmp(grel, "near") != 0)
      {
        orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "invalid geo-relation for Point", 400);
        return false;
      }
      geoInfoP->georel = GeorelNear;

      //
      // Must be: (max|min)Distance==NUMBER
      //
      char* distance = strstr(extra, "==");

      if (distance == NULL)
      {
        orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "no distance for georel 'near' for Point", 400);
        return false;
      }
      *distance = 0;
      distance = &distance[2];

      bool max = false;
      if (strcmp(extra, "maxDistance") == 0)
        max = true;
      else if (strcmp(extra, "minDistance") == 0)
        max = false;
      else
      {
        orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "no distance for georel 'near' for Point", 400);
        return false;
      }

      //
      // 'distance' must be an INTEGER
      //
      char* distanceStart = distance;
      while (*distance != 0)
      {
        if ((*distance < '0') || (*distance > '9'))
        {
          orionldError(OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "invalid number for distance for georel 'near' for Point", 400);
          return false;
        }
        ++distance;
      }
      int dist = atoi(distanceStart);

      if (max)
        geoInfoP->maxDistance = dist;
      else
        geoInfoP->minDistance = dist;
    }
  }

  return true;
}
