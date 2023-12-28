/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include <strings.h>                                           // bzero
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjParse.h"                                     // kjParse
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/urlDecode.h"                          // urlDecode
#include "orionld/types/OrionldGeoInfo.h"                      // OrionldGeoInfo
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/payloadCheck/pCheckGeometry.h"               // pCheckGeometry
#include "orionld/payloadCheck/pCheckGeorelString.h"           // pCheckGeorelString
#include "orionld/payloadCheck/pCheckGeoCoordinates.h"         // pCheckGeoCoordinates
#include "orionld/payloadCheck/pCheckGeo.h"                    // Own interface



// ----------------------------------------------------------------------------
//
// pCheckGeo -
//
bool pCheckGeo(OrionldGeoInfo* geoInfoP, char* geometry, char* georel, char* coordinates, char* geoproperty)
{
  bzero(geoInfoP, sizeof(OrionldGeoInfo));

  if ((geometry    != NULL) ||
      (georel      != NULL) ||
      (coordinates != NULL) ||
      (geoproperty != NULL))
  {
    //
    // geometry
    //
    if (geometry == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: geometry", 400);
      return false;
    }
    if (pCheckGeometry(geometry, &geoInfoP->geometry, false) == false)
      return false;

    //
    // georel
    //
    if (georel == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: georel", 400);
      return false;
    }

    if (pCheckGeorelString(georel, geoInfoP) == false)
      return false;

    //
    // coordinates
    //
    if (coordinates == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: coordinates", 400);
      return false;
    }

    urlDecode(coordinates);
    geoInfoP->coordinates = kjParse(orionldState.kjsonP, coordinates);

    if (geoInfoP->coordinates == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "invalid coordinates", 400);
      return false;
    }

    if (pCheckGeoCoordinates(geoInfoP->coordinates, geoInfoP->geometry) == false)
      return false;


    //
    // geoproperty
    //
    if ((geoproperty != NULL) && (strcmp(geoproperty, "location") != 0))
      geoInfoP->geoProperty = orionldAttributeExpand(orionldState.contextP, geoproperty, true, NULL);
    else
      geoInfoP->geoProperty = (char*) "location";
  }

  return true;
}
