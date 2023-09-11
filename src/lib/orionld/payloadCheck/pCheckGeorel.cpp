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
#include <unistd.h>                                             // NULL

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/types/OrionldGeoInfo.h"                       // OrionldGeoInfo
#include "orionld/payloadCheck/pCheckGeorelString.h"            // pCheckGeorelString
#include "orionld/payloadCheck/pCheckGeorel.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// pCheckGeorel -
//
bool pCheckGeorel(KjNode* georelP, OrionldGeoInfo* geoInfoP)
{
  if (georelP == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Geo-Spatial Filter", "georel missing", 400);
    return false;
  }
  else if (georelP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid Geo-Spatial Filter", "georel must be a string", 400);
    return false;
  }
  else if (georelP->value.s[0] == 0)
  {
    orionldError(OrionldBadRequestData, "Invalid Geo-Spatial Filter", "georel value is empty", 400);
    return false;
  }

  return pCheckGeorelString(georelP->value.s, geoInfoP);
}
