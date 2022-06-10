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
#include "orionld/types/OrionldGeometry.h"                             // OrionldGeometry
#include "orionld/payloadCheck/PCHECK.h"                               // PCHECK_ARRAY
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
bool pCheckGeoCoordinates(KjNode* coordinatesP, OrionldGeometry geometry)
{
  //
  // It's either an Array OR A STRING !!!
  //
  PCHECK_ARRAY(coordinatesP, 0, "Invalid JSON type for /coordinates/ field (not a JSON Array)", kjValueType(coordinatesP->type), 400);

  switch (geometry)
  {
  case GeoPoint:            return pCheckGeoPointCoordinates(coordinatesP);
  case GeoMultiPoint:       return pCheckGeoMultiPointCoordinates(coordinatesP);
  case GeoLineString:       return pCheckGeoLineStringCoordinates(coordinatesP);
  case GeoMultiLineString:  return pCheckGeoMultiLineStringCoordinates(coordinatesP);
  case GeoPolygon:          return pCheckGeoPolygonCoordinates(coordinatesP);
  case GeoMultiPolygon:     return pCheckGeoMultiPolygonCoordinates(coordinatesP);

  default:
    //
    // Shouldn't reach this point, we would have discovered the error already right after the call to pcheckGeoType()
    //
    orionldError(OrionldBadRequestData, "Invalid geometry for GeoJSON", "This can't happen", 400);
    return false;
  }

  return true;
}
