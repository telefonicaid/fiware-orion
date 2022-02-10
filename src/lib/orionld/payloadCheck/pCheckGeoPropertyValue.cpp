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
#include "orionld/types/OrionldGeoJsonType.h"                    // OrionldGeoJsonType
#include "orionld/payloadCheck/pCheckAttributeTransform.h"       // pCheckAttributeTransform
#include "orionld/payloadCheck/pCheckGeoPropertyType.h"          // pCheckGeoPropertyType
#include "orionld/payloadCheck/pCheckGeoPropertyCoordinates.h"   // pCheckGeoPropertyCoordinates
#include "orionld/payloadCheck/pCheckGeoPropertyValue.h"         // Own interface



// -----------------------------------------------------------------------------
//
// pCheckGeoPropertyValue -
//
bool pCheckGeoPropertyValue(KjNode* attrP, KjNode* typeP)
{
  if (typeP == NULL)
  {
    typeP = kjLookup(attrP, "type");

    if (typeP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory Field /type/ missing for a GeoProperty value", attrP->name, 400);
      return false;
    }
  }

  // Is the type a valid GeoJSON type? (Point, Polygon, ...)
  OrionldGeoJsonType geoType;
  if (pCheckGeoPropertyType(typeP, &geoType, attrP->name) == false)  // pCheckGeoPropertyType sets ProblemDetails
    return false;

  KjNode* coordinatesP = kjLookup(attrP, "coordinates");

  if (coordinatesP != NULL)
  {
    if (pCheckGeoPropertyCoordinates(coordinatesP, geoType) == true)  // pCheckGeoPropertyCoordinates sets ProblemDetails
    {
      // Convert to Normalized
      KjNode* valueP = kjObject(orionldState.kjsonP,  "value");

      valueP->value.firstChildP = attrP->value.firstChildP;
      valueP->lastChild         = attrP->lastChild;

      pCheckAttributeTransform(attrP, "GeoProperty", valueP);
      return true;
    }
    else
      return false;
  }

  orionldError(OrionldBadRequestData, "Mandatory Field /coordinates/ missing for a GeoProperty value", attrP->name, 400);
  return false;
}
