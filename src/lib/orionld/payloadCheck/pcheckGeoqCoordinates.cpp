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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/OrionldGeoJsonType.h"                  // OrionldGeoJsonType
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/payloadCheck/pcheckGeoqCoordinates.h"        // Own interface



// -----------------------------------------------------------------------------
//
// pcheckGeoPoint -
//
static bool pcheckGeoPoint(KjNode* geoPointCoordinatesNodeP)
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
// pcheckGeoMultiPoint -
//
static bool pcheckGeoMultiPoint(KjNode* geoPointCoordinatesNodeP)
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

    if (pcheckGeoPoint(memberP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckGeoLineString -
//
static bool pcheckGeoLineString(KjNode* geoPointCoordinatesNodeP)
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
    if (pointP->type != KjArray)
    {
      LM_W(("Bad Input ('coordinates' must be a JSON Array)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'LineString' must be a JSON Array of at least two Arrays");
      return false;
    }

    if (pcheckGeoPoint(pointP) == false)
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
// pcheckGeoMultiLineString -
//
static bool pcheckGeoMultiLineString(KjNode* multiLineStringNodeP)
{
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

    if (pcheckGeoLineString(lineStringNodeP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckGeoPolygon -
//
// This is how a Polygon looks:
//     "value": {
//      "type": "Polygon",
//      "coordinates": [[ [0,0], [4,0], [4,-2], [0,-2], [0,0] ], [ [0,0], ...]]
//    }
//
// I.e. an Array of Arrays of Points
//
static bool pcheckGeoPolygon(KjNode* geoPointCoordinatesNodeP)
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

    if (ringP->type != KjArray)
    {
      LM_W(("Bad Input (one of the rings is not an array"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays");
      return false;
    }

    for (KjNode* memberP = ringP->value.firstChildP; memberP != NULL; memberP = memberP->next)
    {
      if (memberP->type != KjArray)
      {
        LM_W(("Bad Input (a member of Polygon must be a JSON Array)"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays of 'Point'");
        return false;
      }

      if (pcheckGeoPoint(memberP) == false)
      {
        LM_W(("Bad Input (one of the points of one of therings is not a valid point"));
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
// pcheckGeoMultiPolygon -
//
static bool pcheckGeoMultiPolygon(KjNode* geoPointCoordinatesNodeP)
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

    if (pcheckGeoPolygon(memberP) == false)
      return false;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// pcheckGeoqCoordinates -
//
bool pcheckGeoqCoordinates(KjNode* coordinatesP, OrionldGeoJsonType geoType)
{
  switch (geoType)
  {
  case GeoJsonPoint:            return pcheckGeoPoint(coordinatesP);           break;
  case GeoJsonMultiPoint:       return pcheckGeoMultiPoint(coordinatesP);      break;
  case GeoJsonLineString:       return pcheckGeoLineString(coordinatesP);      break;
  case GeoJsonMultiLineString:  return pcheckGeoMultiLineString(coordinatesP); break;
  case GeoJsonPolygon:          return pcheckGeoPolygon(coordinatesP);         break;
  case GeoJsonMultiPolygon:     return pcheckGeoMultiPolygon(coordinatesP);    break;

  default:
    //
    // Shouldn't reach this point, we would have discovered the error already right after the call to pcheckGeoType()
    //
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid geometry for GeoJSON", "This can't happen");
    return false;
    break;
  }

  return true;
}
