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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECKx()
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/types/OrionldGeometry.h"                       // OrionldGeometry
#include "orionld/payloadCheck/pCheckGeometry.h"                 // pCheckGeometry
#include "orionld/payloadCheck/pCheckGeoCoordinates.h"           // pCheckGeoCoordinates
#include "orionld/payloadCheck/pcheckGeoPropertyValue.h"         // Own interface



// ----------------------------------------------------------------------------
//
// pcheckGeoPropertyValue -
//
bool pcheckGeoPropertyValue(KjNode* geoPropertyP, char** geometryPP, KjNode** geoCoordsPP, const char* attrName)
{
  KjNode*             typeNodeP         = NULL;
  KjNode*             coordinatesNodeP  = NULL;
  OrionldGeometry     geometry          = GeoNoGeometry;
  char*               geometryString    = (char*) "NoGeometry";

  if (geoPropertyP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "the value of a geo-location attribute must be a JSON Object", kjValueType(geoPropertyP->type), 400);
    return false;
  }

  for (KjNode* nodeP = geoPropertyP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(typeNodeP, "geo-location::type", nodeP);
      STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object must be a JSON String");
      EMPTY_STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object cannot be an empty string");

      if (pCheckGeometry(typeNodeP->value.s, &geometry, false) == false)
      {
        // orionldError(OrionldBadRequestData, detail, typeNodeP->value.s, 400);
        return false;
      }
      geometryString = typeNodeP->value.s;
    }
    else if (SCOMPARE12(nodeP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(coordinatesNodeP, "geo-location::coordinates", nodeP);
      PCHECK_STRING_OR_ARRAY(coordinatesNodeP, 0, "Invalid Data Type", "the 'coordinates' field of a GeoJSON object must be a JSON Array (or String)", 400);
    }
    else
    {
      orionldError(OrionldBadRequestData, "unexpected item in geo-location", attrName, 400);
      return false;
    }
  }

  if ((typeNodeP == NULL) && (coordinatesNodeP == NULL))
  {
    orionldError(OrionldBadRequestData,
                 "The value of an attribute of type GeoProperty must be valid GeoJson",
                 "Mandatory 'coordinates' and 'type' fields missing for a GeoJSON Property",
                 400);
    return false;
  }

  if (typeNodeP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory 'type' field missing for GeoJSON Property", attrName, 400);
    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    orionldError(OrionldBadRequestData,
                 "The value of an attribute of type GeoProperty must be valid GeoJson - 'coordinates' field missing",
                 attrName,
                 400);
    return false;
  }

  // Check that the coordinates (coordinatesNodeP) JSON Array coincides with the type 'geometry'
  if (coordinatesNodeP->type == KjString)
  {
    coordinatesNodeP = kjParse(orionldState.kjsonP, coordinatesNodeP->value.s);
    if (coordinatesNodeP == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid GeoJSON", "'coordinates' as string is not a valid JSON Array", 400);
      return false;
    }
  }

  if (pCheckGeoCoordinates(coordinatesNodeP, geometry) == false)
    return false;

  //
  // Keep coordsP and typeP for later use (in mongoBackend)
  //
  if (geometryPP != NULL)
    *geometryPP   = geometryString;
  if (geoCoordsPP != NULL)
    *geoCoordsPP = coordinatesNodeP;

  return true;
}
