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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/OrionldGeometry.h"                       // OrionldGeometry
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_DUPLICATE
#include "orionld/payloadCheck/pCheckGeoPropertyType.h"          // pCheckGeoPropertyType
#include "orionld/payloadCheck/pCheckGeoCoordinates.h"           // pCheckGeoCoordinates
#include "orionld/payloadCheck/pCheckGeoPropertyValue.h"         // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeoPropertyValue -
//
bool pCheckGeoPropertyValue(KjNode* valueP, const char* attrLongName)
{
  KjNode* typeP        = NULL;
  KjNode* coordinatesP = NULL;

  if (valueP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "The value of a GeoProperty must be a JSON Object", attrLongName, 400);
    return false;
  }

  for (KjNode* itemP = valueP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(itemP->name, "type") == 0)
      PCHECK_DUPLICATE(typeP, itemP, 0, NULL, "type field in value of GeoProperty", 400);
    else if (strcmp(itemP->name, "coordinates") == 0)
      PCHECK_DUPLICATE(coordinatesP, itemP, 0, NULL, "coordinates field in value of GeoProperty", 400);
    else
    {
      orionldError(OrionldBadRequestData, "Unexpected Field in value of GeoProperty", itemP->name, 400);
      return false;
    }
  }

  if (typeP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory Field /type/ missing for a GeoProperty value", attrLongName, 400);
    return false;
  }

  if (coordinatesP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory Field /coordinates/ missing for a GeoProperty value", attrLongName, 400);
    return false;
  }

  // Is the type a valid GeoJSON type? (Point, Polygon, ...)
  OrionldGeometry geometry;
  // FIXME: Rename to pCheckGeoPropertyValueType?
  if (pCheckGeoPropertyType(typeP, &geometry, attrLongName) == false)  // pCheckGeoPropertyType sets ProblemDetails
    return false;

  // FIXME: Rename to pCheckGeoPropertyValueCoordinates?
  if (pCheckGeoCoordinates(coordinatesP, geometry) == false)  // pCheckGeoCoordinates sets ProblemDetails
    return false;

  return true;
}
