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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/OrionldGeoJsonType.h"                  // OrionldGeoJsonType
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/geoJsonTypeCheck.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// geoJsonTypeCheck -
//
bool geoJsonTypeCheck(char* typeName, OrionldGeoJsonType* typeP, char** detailsP)
{
  if      (SCOMPARE6(typeName,  'P', 'o', 'i', 'n', 't', 0))                                                      *typeP = GeoJsonPoint;
  else if (SCOMPARE11(typeName, 'M', 'u', 'l', 't', 'i', 'P', 'o', 'i', 'n', 't', 0))                             *typeP = GeoJsonMultiPoint;
  else if (SCOMPARE11(typeName, 'L', 'i', 'n', 'e', 'S', 't', 'r', 'i', 'n', 'g', 0))                             *typeP = GeoJsonLineString;
  else if (SCOMPARE16(typeName, 'M', 'u', 'l', 't', 'i', 'L', 'i', 'n', 'e', 'S', 't', 'r', 'i', 'n', 'g', 0))    *typeP = GeoJsonMultiLineString;
  else if (SCOMPARE8(typeName,  'P', 'o', 'l', 'y', 'g', 'o', 'n', 0))                                            *typeP = GeoJsonPolygon;
  else if (SCOMPARE13(typeName, 'M', 'u', 'l', 't', 'i', 'P', 'o', 'l', 'y', 'g', 'o', 'n', 0))                   *typeP = GeoJsonMultiPolygon;
  else
  {
    LM_E(("Invalid GeoJSON type: %s", typeName));
    *detailsP = (char*) "invalid GeoJSON type";

    return false;
  }

  return true;
}
