/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/types/OrionldProblemDetails.h"                // OrionldProblemDetails
#include "orionld/types/OrionldGeoJsonType.h"                   // OrionldGeoJsonType



// ----------------------------------------------------------------------------
//
// pcheckGeoqGeorel -
//
bool pcheckGeoqGeorel(KjNode* georelP, OrionldGeoJsonType geoType, OrionldProblemDetails* pdP)
{
  if (georelP == NULL)
  {
    orionldProblemDetailsFill(pdP, OrionldInvalidRequest, "Invalid Geo-Spatial Filter", "georel missing", 400);
    return false;
  }
  else if (georelP->type != KjString)
  {
    orionldProblemDetailsFill(pdP, OrionldInvalidRequest, "Invalid Geo-Spatial Filter", "georel must be a string", 400);
    return false;
  }
  else if (georelP->value.s[0] == 0)
  {
    orionldProblemDetailsFill(pdP, OrionldInvalidRequest, "Invalid Geo-Spatial Filter", "georel value is empty", 400);
    return false;
  }

  char* georel = georelP->value.s;

  LM_TMP(("GEO: georel: %s", georel));

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
    if (geoType != GeoJsonPoint)
    {
      orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "Geometry must be 'Point' for georel 'near'", 400);
      return false;
    }
  }
  else if (strcmp(georel, "within") == 0)
  {
    if ((geoType != GeoJsonPolygon) && (geoType != GeoJsonMultiPolygon))
    {
      orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "Geometry must be either 'Polygon' or 'MultiPolygon' for georel 'within'", 400);
      return false;
    }
  }
  else if (strcmp(georel, "overlaps") == 0)
  {
    // FIXME: Can be any Geometry ... Right?
  }
  else if (strcmp(georel, "intersects") == 0)
  {
    // FIXME: Should be possible to use any Geomtry ...
    //        But, the mngodb docs talk about Polygon or MultiPolygon:
    //          https://docs.mongodb.com/manual/reference/operator/query/geoIntersects/
    //        For now, I'll allow any Geometry
  }
  else if (strcmp(georel, "disjoint") == 0)
  {
    // Uses $not { 'intersects-expression' }, so, if 'intersects' is only Polygons, then 'disjoint' also
  }
  else if (strcmp(georel, "contains") == 0)
  {
    orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Not Inplemented", "georel 'contains' is not supported by mongodb and thus also not by Orion-LD", 501);
    return false;
  }
  else if (strcmp(georel, "equals") == 0)
  {
    // Valid for all geometry types
  }
  else
  {
    orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter - invalid 'georel'", georel, 400);
    return false;
  }

  if (geoType == GeoJsonPoint)
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
        orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "invalid geo-relation for Point", 400);
        return false;
      }

      //
      // Must be: (max|min)Distance==NUMBER
      //
      char* distance = strstr(extra, "==");

      if (distance == NULL)
      {
        orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "no distance for georel 'near' for Point", 400);
        return false;
      }
      *distance = 0;
      distance = &distance[2];

      if ((strcmp(extra, "maxDistance") != 0) && (strcmp(extra, "minDistance") != 0))
      {
        orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "no distance for georel 'near' for Point", 400);
        return false;
      }

      //
      // 'distance' must be an INTEGER
      //
      while (*distance != 0)
      {
        if ((*distance < '0') || (*distance > '9'))
        {
          orionldProblemDetailsFill(pdP, OrionldInvalidRequest,  "Invalid Geo-Spatial filter", "invalid number for distance for georel 'near' for Point", 400);
          return false;
        }
        ++distance;
      }
    }
  }

  return true;
}
