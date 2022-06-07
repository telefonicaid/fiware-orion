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
#include <string.h>                               // strcmp

#include "orionld/types/OrionldGeometry.h"        // Own Interface



// -----------------------------------------------------------------------------
//
// orionldGeometryFromString -
//
OrionldGeometry orionldGeometryFromString(const char* geoString)
{
  if      (strcmp(geoString, "Point")           == 0)  return GeoPoint;
  else if (strcmp(geoString, "Polygon")         == 0)  return GeoPolygon;
  else if (strcmp(geoString, "LineString")      == 0)  return GeoLineString;
  else if (strcmp(geoString, "MultiPoint")      == 0)  return GeoMultiPoint;
  else if (strcmp(geoString, "MultiPolygon")    == 0)  return GeoMultiPolygon;
  else if (strcmp(geoString, "MultiLineString") == 0)  return GeoMultiLineString;

  return GeoNoGeometry;
}



// -----------------------------------------------------------------------------
//
// orionldGeometryFromString -
//
const char* orionldGeometryToString(OrionldGeometry geometry)
{
  switch (geometry)
  {
  case GeoPoint:           return "Point";
  case GeoPolygon:         return "Polygon";
  case GeoLineString:      return "LineString";
  case GeoMultiPoint:      return "MultiPoint";
  case GeoMultiPolygon:    return "MultiPolygon";
  case GeoMultiLineString: return "MultiLineString";
  case GeoNoGeometry:      return "No-Geometry";
  }

  return "No Geometry";
}
