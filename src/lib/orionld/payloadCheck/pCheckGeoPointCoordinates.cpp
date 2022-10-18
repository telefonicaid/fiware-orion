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
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldBadRequestData
#include "orionld/payloadCheck/pCheckGeoPointCoordinates.h"      // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeoPointCoordinates -
//
bool pCheckGeoPointCoordinates(KjNode* coordinatesP)
{
  // Must be an array of Number, two or three members
  int     numbers   = 0;
  double  latitude  = 0;
  double  longitude = 0;

  for (KjNode* memberP = coordinatesP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    if ((memberP->type == KjInt) || (memberP->type == KjFloat))
      ++numbers;
    else
    {
      orionldError(OrionldBadRequestData, "Invalid GeoJSON", "all members of a 'Point' in 'coordinates' must be of type 'Number'", 400);
      return false;
    }

    if      (numbers == 1)  longitude = (memberP->type == KjFloat)? memberP->value.f : memberP->value.i;
    else if (numbers == 2)  latitude  = (memberP->type == KjFloat)? memberP->value.f : memberP->value.i;
  }

  if ((numbers != 2) && (numbers != 3))
  {
    orionldError(OrionldBadRequestData, "Invalid GeoJSON", "The 'coordinates' member for a 'Point' must be an array with two or three Numbers", 400);
    return false;
  }

  if ((latitude < -90) || (latitude > 90))
  {
    orionldError(OrionldBadRequestData, "Invalid GeoJSON", "latitude outside of range -90 to 90", 400);
    return false;
  }

  if ((longitude < -180) || (longitude > 180))
  {
    orionldError(OrionldBadRequestData, "Invalid GeoJSON", "longitude outside of range -180 to 180", 400);
    return false;
  }

  return true;
}
