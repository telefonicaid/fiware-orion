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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckGeoPointCoordinates.h"      // pCheckGeoPointCoordinates
#include "orionld/payloadCheck/pCheckGeoPolygonCoordinates.h"    // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeoPolygonCoordinates -
//
// This is how a Polygon looks:
//     "value": {
//      "type": "Polygon",
//      "coordinates": [[ [0,0], [4,0], [4,-2], [0,-2], [0,0] ], [ [0,0], ...]]
//    }
//
// I.e. an Array of Arrays of Points
//
bool pCheckGeoPolygonCoordinates(KjNode* coordinatesP)
{
  //
  // A Polygon contains "rings" and a ring is an array of points
  //
  int ringNo = 0;
  for (KjNode* ringP = coordinatesP->value.firstChildP; ringP != NULL; ringP = ringP->next)
  {
    KjNode* firstPosP = NULL;
    KjNode* lastPosP  = NULL;
    int     points    = 0;

    if (ringP->type != KjArray)
    {
      LM_W(("Bad Input (one of the rings is not an array"));
      orionldError(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays", 400);
      return false;
    }

    for (KjNode* memberP = ringP->value.firstChildP; memberP != NULL; memberP = memberP->next)
    {
      if (memberP->type != KjArray)
      {
        LM_W(("Bad Input (a member of Polygon must be a JSON Array)"));
        orionldError(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays of 'Point'", 400);
        return false;
      }

      if (pCheckGeoPointCoordinates(memberP) == false)
      {
        LM_W(("Bad Input (one of the points of one of therings is not a valid point"));
        orionldError(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' in a 'Polygon' must be a JSON Array of 'Rings' that are JSON Arrays of 'Point'", 400);
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
      orionldError(OrionldBadRequestData, "Invalid GeoJSON", "A Polygon must have at least 4 points", 400);
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
        orionldError(OrionldBadRequestData, "Invalid GeoJSON", "In a Polygon, the first and the last position must be identical", 400);
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
      orionldError(OrionldBadRequestData, "Invalid GeoJSON", "In a Polygon, the first and the last position must be identical", 400);
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
