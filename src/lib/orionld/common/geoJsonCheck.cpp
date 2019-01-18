/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECKx(U)
#include "orionld/common/OrionldGeoJsonType.h"                 // OrionldGeoJsonType
#include "orionld/common/geoJsonTypeCheck.h"                   // geoJsonTypeCheck
#include "orionld/common/geoJsonCheck.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// geoJsonCheck - check validity of any geo-property
//
// The value of a GeoJSON Property must be a JSON Object.
// The Object must have exactly two members:
// - type:        a JSON String that is a supported GeoJSON type (Point, Polygon, ...)
// - coordinates: a JSON Array of Numbers. The number of members depends on the 'type'
//
//
// ACCEPTED TYPES:
// First of all, a Position is an Array of 2-3 Numbers  (Implementations SHOULD NOT extend positions beyond three elements)
//
//   Point:            a Position                        [ 1, 2 ]
//   MultiPoint:       an Array of Positions             [ [ 1, 2 ], [ 1, 2 ], [ 1, 2 ], ... ]
//   LineString:       an Array of 2+ Positions          [ [ 1, 2 ], [ 1, 2 ], ... ]
//   MultiLineString:  an Array of LineString            [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ]
//   Polygon:          closed LineString with 4+ pos     [ [ 1, 2 ], [ 3, 4 ], [ 5, 6 ], ..., [ 1, 2 ] ]
//   MultiPolygon:     an Array of Polygon               [ [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ], [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ], ... ]
//
bool geoJsonCheck(ConnectionInfo* ciP, KjNode* geoJsonNodeP, char** detailsP)
{
  KjNode*            typeP        = NULL;
  KjNode*            coordsP      = NULL;
  OrionldGeoJsonType geoJsonType  = (GeoJsonType) -1;

  if (geoJsonNodeP->type != KjObject)
  {
    *detailsP = (char*) "A GeoProperty value must be a JSON Object";
    return false;
  }

  for (KjNode* itemP = geoJsonNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    LM_T(LmtGeoJson, ("Item '%s'", itemP->name));

    if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(typeP, geoJsonNodeP->name, itemP);

      if (itemP->type != KjString)
      {
        *detailsP = (char*) "the 'type' field of a GeoJSON object must be a JSON String";
        return false;
      }

      if (geoJsonTypeCheck(itemP->value.s, &geoJsonType, detailsP) == false)
        return false;
    }
    else if (SCOMPARE12(itemP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(coordsP, geoJsonNodeP->name, itemP);

      if (itemP->type != KjArray)
      {
        *detailsP = (char*) "the 'coordinates' field of a GeoJSON object must be a JSON Array";
        return false;
      }
    }
    else
    {
      *detailsP = (char*) "invalid field in a GeoJSON object";
      return false;
    }
  }

  if (typeP == NULL)
  {
    *detailsP = (char*) "Mandatory 'type' field missing for a GeoJSON Property";
    return false;
  }

  if (coordsP == NULL)
  {
    *detailsP = (char*) "Mandatory 'coordinates' field missing for a GeoJSON Property";
    return false;
  }

  // FIXME: Check all types of GeoJSON - Point, Polygon, etc (different Arrays of Number)
  // if (geoJsonCoordinatesCheck(coordsP->value.firstChildP, detailsP) == false)
  //   return false;

  return true;
}
