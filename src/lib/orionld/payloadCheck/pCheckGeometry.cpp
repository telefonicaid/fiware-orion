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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/OrionldGeometry.h"                     // OrionldGeometry, GeoPoint, GeoLineString, ...
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/payloadCheck/pCheckGeometry.h"               // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeometry -
//
bool pCheckGeometry(char* geometryString, OrionldGeometry* geometryP, bool isSubscription)
{
  if (geometryString[0] == 0)
  {
    orionldError(OrionldBadRequestData, "Invalid geometry", "empty string", 400);
    return false;
  }

  if      (strcmp(geometryString, "Point")           == 0) *geometryP = GeoPoint;
  else if (strcmp(geometryString, "MultiPoint")      == 0) *geometryP = GeoMultiPoint;
  else if (strcmp(geometryString, "LineString")      == 0) *geometryP = GeoLineString;
  else if (strcmp(geometryString, "MultiLineString") == 0) *geometryP = GeoMultiLineString;
  else if (strcmp(geometryString, "Polygon")         == 0) *geometryP = GeoPolygon;
  else if (strcmp(geometryString, "MultiPolygon")    == 0) *geometryP = GeoMultiPolygon;
  else
  {
    orionldError(OrionldBadRequestData, "Invalid geometry", geometryString, 400);
    return false;
  }

  if ((isSubscription == true) && (*geometryP != GeoPoint))
  {
    orionldError(OrionldOperationNotSupported, "Not Implemented", "Subscriptions only support Point for geometry (right now)", 501);
    return false;
  }

  return true;
}
