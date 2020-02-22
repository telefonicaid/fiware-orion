/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
// orionldCheckGeoJsonPoint -
//
bool orionldCheckGeoJsonPoint(ConnectionInfo* ciP, KjNode* geoPointCoordinatesNodeP)
{
  int numbers = 0;

  if (geoPointCoordinatesNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  // Must be an array of Number, two or three members
  for (KjNode* memberP = geoPointCoordinatesNodeP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    if ((memberP->type == KjInt) || (memberP->type == KjFloat))
      ++numbers;
    else
    {
      LM_W(("Bad Input (member of 'coordinates' array is not a number but of type: '%s')", kjValueType(memberP->type)));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "all members of a 'Point' in 'coordinates' must be of type 'Number'");
      return false;
    }
  }

  if ((numbers != 2) && (numbers != 3))
  {
    LM_W(("Bad Input (invalid coordinates: %d numbers in array)", numbers));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "The 'coordinates' member for a 'Point' must be an array with two or three Numbers");
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonMultiPoint -
//
bool orionldCheckGeoJsonMultiPoint(ConnectionInfo* ciP, KjNode* geoPointCoordinatesNodeP)
{
  if (geoPointCoordinatesNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  // Must be an array of arrays (that are of the type 'Point')
  for (KjNode* memberP = geoPointCoordinatesNodeP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    if (memberP->type != KjArray)
    {
      LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'MultiPoint' must be a JSON Array of Arrays");
      return false;
    }

    if (orionldCheckGeoJsonPoint(ciP, memberP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonLineString -
//
bool orionldCheckGeoJsonLineString(ConnectionInfo* ciP, KjNode* geoPointCoordinatesNodeP)
{
  if (geoPointCoordinatesNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  // Must be an array of at least two arrays (that are of the type 'Point')
  int arrays = 0;
  for (KjNode* pointP = geoPointCoordinatesNodeP->value.firstChildP; pointP != NULL; pointP = pointP->next)
  {
    LM_TMP(("MLS: Point at %p is a JSON %s", pointP, kjValueType(pointP->type)));
    if (pointP->type != KjArray)
    {
      LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'LineString' must be a JSON Array of at least two Arrays");
      return false;
    }

    if (orionldCheckGeoJsonPoint(ciP, pointP) == false)
      return false;
    ++arrays;
  }

  if (arrays < 2)
  {
    LM_W(("Bad Input ('coordinates' in a 'LineString' must be a JSON Array of at least two Arrays)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'LineString' must be a JSON Array of at least two Arrays");
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonMultiLineString -
//
bool orionldCheckGeoJsonMultiLineString(ConnectionInfo* ciP, KjNode* multiLineStringNodeP)
{
  LM_TMP(("MLS: MultiLineString at %p is a JSON %s", multiLineStringNodeP, kjValueType(multiLineStringNodeP->type)));
  if (multiLineStringNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  for (KjNode* lineStringNodeP = multiLineStringNodeP->value.firstChildP; lineStringNodeP != NULL; lineStringNodeP = lineStringNodeP->next)
  {
    if (lineStringNodeP->type != KjArray)
    {
      LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'MultiLineString' must be a JSON Array of 'LineString arrays'");
      return false;
    }

    LM_TMP(("MLS: LineString at %p is a JSON %s", lineStringNodeP, kjValueType(lineStringNodeP->type)));
    if (orionldCheckGeoJsonLineString(ciP, lineStringNodeP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonPolygon -
//
// This is how a Polygon looks:
//     "value": {
//      "type": "Polygon",
//      "coordinates": [[ [0,0], [4,0], [4,-2], [0,-2], [0,0] ], [ [0,0], ...]]
//    }
//
// I.e. an Array of Arrays of Points
//
bool orionldCheckGeoJsonPolygon(ConnectionInfo* ciP, KjNode* geoPointCoordinatesNodeP)
{
  if (geoPointCoordinatesNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  //
  // A Polygon contains "rings" and a ring is an array of points
  //
  int ringNo = 0;
  for (KjNode* ringP = geoPointCoordinatesNodeP->value.firstChildP; ringP != NULL; ringP = ringP->next)
  {
    KjNode* firstPosP = NULL;
    KjNode* lastPosP  = NULL;
    int     points    = 0;

    for (KjNode* memberP = ringP->value.firstChildP; memberP != NULL; memberP = memberP->next)
    {
      if (memberP->type != KjArray)
      {
        LM_W(("Bad Input (a member of Polygon must be a JSON Array)"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays of 'Point'");
        return false;
      }

      if (points == 0)
        firstPosP = memberP;
      lastPosP = memberP;

      ++points;
    }

    //
    // A polygon must have at least 4 points
    //
    if (points < 4)
    {
      LM_W(("Bad Input (A Polygon must have at least 4 points)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "A Polygon must have at least 4 points");
      return false;
    }

    //
    // The first position and the last position must be identical
    //
    KjNode* firstItemNodeP = firstPosP->value.firstChildP;
    KjNode* lastItemNodeP  = lastPosP->value.firstChildP;
    int     index          = 0;

    while (firstItemNodeP != NULL)
    {
      if ((lastItemNodeP == NULL) || (firstItemNodeP->value.f != lastItemNodeP->value.f))
      {
        LM_W(("Bad Input (In a Polygon, the first and the last position must be identical)"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "In a Polygon, the first and the last position must be identical");
        return false;
      }

      ++index;
      firstItemNodeP = firstItemNodeP->next;
      lastItemNodeP  = lastItemNodeP->next;
    }

    // Now both firstItemNodeP and lastItemNodeP must be NULL - we already know that firstItemNodeP is NULL - that's when the loop ended
    if (lastItemNodeP != NULL)
    {
      LM_W(("Bad Input (In a Polygon, the first and the last position must be identical)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "In a Polygon, the first and the last position must be identical");
      return false;
    }

    //
    // FIXME: If ringNo == 0, the ring must be counterclockwise
    //        Else: clockwise
    //

    ++ringNo;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonMultiPolygon -
//
bool orionldCheckGeoJsonMultiPolygon(ConnectionInfo* ciP, KjNode* geoPointCoordinatesNodeP)
{
  if (geoPointCoordinatesNodeP->type != KjArray)
  {
    LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be a JSON Array");
    return false;
  }

  for (KjNode* memberP = geoPointCoordinatesNodeP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    if (memberP->type != KjArray)
    {
      LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'MultiPolygon' must be a JSON Array of 'Polygon arrays'");
      return false;
    }

    if (orionldCheckGeoJsonPolygon(ciP, memberP) == false)
      return false;
  }

  return true;
}



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

  LM_TMP(("MLS: In geoJsonCheck, geoLocationNodeP->name: %s", geoLocationNodeP->name));
  if (geoLocationNodeP->type != KjObject)
  {
    LM_W(("Bad Input (the value of a geo-location attribute must be a JSON Object: '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData, "the value of a geo-location attribute must be a JSON Object", kjValueType(geoLocationNodeP->type));
    return false;
  }

  for (KjNode* nodeP = geoLocationNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      char* details;

      DUPLICATE_CHECK(typeNodeP, "geo-location::type", nodeP);
      STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object must be a JSON String");

      LM_TMP(("MLS: Calling geoJsonTypeCheck for '%s'", typeNodeP->value.s));
      if (geoJsonTypeCheck(typeNodeP->value.s, &geoType, &details) == false)
      {
        LM_W(("Bad Input (invalid type for geoJson: '%s')", typeNodeP->value.s));
        orionldErrorResponseCreate(OrionldBadRequestData, details, typeNodeP->value.s);
        return false;
      }
      geoTypeString = typeNodeP->value.s;
      LM_TMP(("MLS: geoJsonTypeCheck returned %d for '%s'", geoType, geoTypeString));
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
      orionldErrorResponseCreate(OrionldBadRequestData, "unexpected item in geo-location", geoLocationNodeP->name);
      return false;
    }
  }

  if ((typeNodeP == NULL) && (coordinatesNodeP == NULL))
  {
    LM_W(("Bad Input (empty value in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' and 'type' fields missing for a GeoJSON Property");
    return false;
  }

  if (typeNodeP == NULL)
  {
    LM_W(("Bad Input ('type' missing in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "Mandatory 'type' field missing for GeoJSON Property",
                               geoLocationNodeP->name);

    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    LM_W(("Bad Input ('coordinates' missing in geo-location '%s')", geoLocationNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' field missing for a GeoJSON Property");
    return false;
  }

  // Now we check that the coordinates (coordinatesNodeP) JSON Array coincides with the type 'geoType'
  bool result = false;

  LM_TMP(("MLS: Got geoType: %d", geoType));
  switch (geoType)
  {
  case GeoJsonPoint:            result = orionldCheckGeoJsonPoint(ciP,           coordinatesNodeP); break;
  case GeoJsonMultiPoint:       result = orionldCheckGeoJsonMultiPoint(ciP,      coordinatesNodeP); break;
  case GeoJsonLineString:       result = orionldCheckGeoJsonLineString(ciP,      coordinatesNodeP); break;
  case GeoJsonMultiLineString:  result = orionldCheckGeoJsonMultiLineString(ciP, coordinatesNodeP); break;
  case GeoJsonPolygon:          result = orionldCheckGeoJsonPolygon(ciP,         coordinatesNodeP); break;
  case GeoJsonMultiPolygon:     result = orionldCheckGeoJsonMultiPolygon(ciP,    coordinatesNodeP); break;

  default:
    //
    // Can't reach this point, we would have discovered the error already right after the call to geoJsonTypeCheck()
    //
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid type for GeoJSON", "This can't happen");
    result = false;
    break;
  }

  if (result == false)
  {
    ciP->httpStatusCode = SccBadRequest;
    return false;
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
