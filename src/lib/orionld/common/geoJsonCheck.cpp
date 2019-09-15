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
#include "orionld/types/OrionldGeoJsonType.h"                  // OrionldGeoJsonType
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECKx(U)
#include "orionld/common/OrionldConnection.h"                  // orionldState
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
bool geoJsonCheck(ConnectionInfo* ciP, KjNode* geoLocationNodeP, char** geoTypePP, KjNode** geoCoordsPP)
{
  KjNode*             typeNodeP         = NULL;
  KjNode*             coordinatesNodeP  = NULL;
  OrionldGeoJsonType  geoType           = GeoJsonNoType;
  char*               geoTypeString     = (char*) "NoGeometry";

  if (geoLocationNodeP->type != KjObject)
  {
    LM_W(("Bad Input (the value of a geo-location attribute must be a JSON Object: '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData, "the value of a geo-location attribute must be a JSON Object", kjValueType(geoLocationNodeP->type), OrionldDetailString);
    return false;
  }

  for (KjNode* nodeP = geoLocationNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      char* details;

      DUPLICATE_CHECK(typeNodeP, "geo-location::type", nodeP);
      STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object must be a JSON String");

      if (geoJsonTypeCheck(typeNodeP->value.s, &geoType, &details) == false)
      {
        LM_W(("Bad Input (invalid type for geoJson: '%s')", typeNodeP->value.s));
        orionldErrorResponseCreate(OrionldBadRequestData, details, typeNodeP->value.s, OrionldDetailString);
        return false;
      }

      geoTypeString = typeNodeP->value.s;
    }
    else if (SCOMPARE12(nodeP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(coordinatesNodeP, "geo-location::coordinates", nodeP);
      // Point, LineString, Polygon ... in the first level they're all Arrays
      ARRAY_CHECK(coordinatesNodeP, "the 'coordinates' field of a GeoJSON object must be a JSON Array");
    }
    else
    {
      LM_W(("Bad Input (unexpected item in geo-location: '%s')", geoLocationNodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "unexpected item in geo-location", geoLocationNodeP->name, OrionldDetailString);
      return false;
    }
  }

  if ((typeNodeP == NULL) && (coordinatesNodeP == NULL))
  {
    LM_W(("Bad Input (empty value in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' and 'type' fields missing for a GeoJSON Property",
                               OrionldDetailString);
    return false;
  }

  if (typeNodeP == NULL)
  {
    LM_W(("Bad Input ('type' missing in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'type' field missing for a GeoJSON Property",
                               OrionldDetailString);

    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    LM_W(("Bad Input ('coordinates' missing in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' field missing for a GeoJSON Property",
                               OrionldDetailString);
    return false;
  }

  // Now we check that the coordinates (coordinatesNodeP) JSON Array coincides with the type 'geoType'
  int numbers = 0;
  int arrays  = 0;

  switch (geoType)
  {
  case GeoJsonPoint:
    // Must be an array of Number, and at least two members
    // Or, an array of arrays
    for (KjNode* memberP = coordinatesNodeP->value.firstChildP; memberP != NULL; memberP = memberP->next)
    {
      if ((memberP->type == KjInt) || (memberP->type == KjFloat))
      {
        ++numbers;
      }
      else if (memberP->type == KjArray)
      {
        ++arrays;
      }
      else
      {
        LM_W(("Bad Input (member of 'coordinates' array is not a number but of type: '%s')", kjValueType(memberP->type)));
        orionldErrorResponseCreate(OrionldBadRequestData, "all members of 'coordinates' must be of type 'Number'", geoLocationNodeP->name, OrionldDetailString);
        return false;
      }
    }

    if ((numbers != 0) && (arrays != 0))
    {
      LM_W(("Bad Input (invalid coordinates)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "invalid value of 'coordinates'", geoLocationNodeP->name, OrionldDetailString);
      return false;
    }

    if ((numbers == 0) && (arrays == 0))
    {
      LM_W(("Bad Input (empty array for 'coordinates')"));
      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "The value of an attribute of type GeoProperty must be valid GeoJson",
                                 "empty array for 'coordinates'",
                                 OrionldDetailString);
      return false;
    }

    if (numbers == 1)
    {
      LM_W(("Bad Input (array with only ONE member for 'coordinates')"));
      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "The value of an attribute of type GeoProperty must be valid GeoJson",
                                 "array with only ONE member for 'coordinates'",
                                 OrionldDetailString);
      return false;
    }
    break;

  case GeoJsonMultiPoint:
    break;
  case GeoJsonLineString:
    break;
  case GeoJsonMultiLineString:
    break;
  case GeoJsonPolygon:
    break;
  case GeoJsonMultiPolygon:
    break;

  default:
    break;
  }

  //
  // Keep coordsP and typeP for later use (in mongoBackend)
  //
  if (geoTypePP != NULL)
    *geoTypePP   = geoTypeString;
  if (geoCoordsPP != NULL)
    *geoCoordsPP = coordinatesNodeP;

  return true;
}
