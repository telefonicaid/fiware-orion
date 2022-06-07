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
#include <string.h>                                      // strcmp

#include "orionld/types/OrionldGeorel.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// orionldGeorelFromString -
//
OrionldGeorel orionldGeorelFromString(const char* georelString)
{
  if      (strcmp(georelString, "near")       == 0) return GeorelNear;
  else if (strcmp(georelString, "within")     == 0) return GeorelWithin;
  else if (strcmp(georelString, "intersects") == 0) return GeorelIntersects;
  else if (strcmp(georelString, "equals")     == 0) return GeorelEquals;
  else if (strcmp(georelString, "disjoint")   == 0) return GeorelDisjoint;
  else if (strcmp(georelString, "overlaps")   == 0) return GeorelOverlaps;

  return GeorelNone;
}


// -----------------------------------------------------------------------------
//
// orionldGeorelFromString -
//
const char* orionldGeorelToString(OrionldGeorel georel)
{
  switch (georel)
  {
  case GeorelNone:        return "no-georel";
  case GeorelNear:        return "near";
  case GeorelWithin:      return "within";
  case GeorelIntersects:  return "intersects";
  case GeorelEquals:      return "equals";
  case GeorelDisjoint:    return "disjoint";
  case GeorelOverlaps:    return "overlaps";
  }

  return "no georel";
}
