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
#include "kjson/KjNode.h"                                              // KjNode
#include "kjson/kjParse.h"                                             // kjParse
}

#include "logMsg/logMsg.h"                                             // LM_*
#include "logMsg/traceLevels.h"                                        // Lmt*

#include "orionld/common/orionldState.h"                               // orionldState
#include "orionld/common/orionldError.h"                               // orionldError
#include "orionld/types/OrionldGeoJsonType.h"                          // OrionldGeoJsonType
#include "orionld/payloadCheck/pCheckGeoPointCoordinates.h"            // pCheckGeoPointCoordinates
#include "orionld/payloadCheck/pCheckGeoMultiPointCoordinates.h"       // pCheckGeoMultiPointCoordinates
#include "orionld/payloadCheck/pCheckGeoLineStringCoordinates.h"       // pCheckGeoLineStringCoordinates
#include "orionld/payloadCheck/pCheckGeoMultiLineStringCoordinates.h"  // pCheckGeoMultiLineStringCoordinates
#include "orionld/payloadCheck/pCheckGeoPolygonCoordinates.h"          // pCheckGeoPolygonCoordinates
#include "orionld/payloadCheck/pCheckGeoMultiPolygonCoordinates.h"     // pCheckGeoMultiPolygonCoordinates
#include "orionld/payloadCheck/pCheckGeoCoordinates.h"                 // Own interface



// ----------------------------------------------------------------------------
//
// pCheckGeoCoordinates -
//
bool pCheckGeoCoordinates(KjNode* coordinatesP, OrionldGeoJsonType geoType)
{
  //
  // It's either an Array OR A STRING !!!
  //
  if (coordinatesP->type != KjArray)
  {
    orionldError(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' must be an Array", 400);
    return false;
  }

  switch (geoType)
  {
  case GeoJsonPoint:            return pCheckGeoPointCoordinates(coordinatesP);           break;
  case GeoJsonMultiPoint:       return pCheckGeoMultiPointCoordinates(coordinatesP);      break;
  case GeoJsonLineString:       return pCheckGeoLineStringCoordinates(coordinatesP);      break;
  case GeoJsonMultiLineString:  return pCheckGeoMultiLineStringCoordinates(coordinatesP); break;
  case GeoJsonPolygon:          return pCheckGeoPolygonCoordinates(coordinatesP);         break;
  case GeoJsonMultiPolygon:     return pCheckGeoMultiPolygonCoordinates(coordinatesP);    break;

  default:
    //
    // Shouldn't reach this point, we would have discovered the error already right after the call to pcheckGeoType()
    //
    orionldError(OrionldBadRequestData, "Invalid geometry for GeoJSON", "This can't happen", 400);
    return false;
    break;
  }

  return true;
}
