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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/types/OrionldGeoJsonType.h"                    // OrionldGeoJsonType
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECKx()
#include "orionld/payloadCheck/pcheckGeoType.h"                  // pcheckGeoType
#include "orionld/payloadCheck/pcheckGeoqCoordinates.h"          // pcheckGeoqCoordinates
#include "orionld/payloadCheck/pcheckGeoProperty.h"              // Own interface



// ----------------------------------------------------------------------------
//
// pcheckGeoProperty -
//
bool pcheckGeoProperty(ConnectionInfo* ciP, KjNode* geoPropertyP, char** geoTypePP, KjNode** geoCoordsPP)
{
  KjNode*             typeNodeP         = NULL;
  KjNode*             coordinatesNodeP  = NULL;
  OrionldGeoJsonType  geoType           = GeoJsonNoType;
  char*               geoTypeString     = (char*) "NoGeometry";

  if (geoPropertyP->type != KjObject)
  {
    LM_W(("Bad Input (the value of a geo-location attribute must be a JSON Object: '%s')", geoPropertyP->name));
    orionldErrorResponseCreate(OrionldBadRequestData, "the value of a geo-location attribute must be a JSON Object", kjValueType(geoPropertyP->type));
    return false;
  }

  for (KjNode* nodeP = geoPropertyP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      char* details;

      DUPLICATE_CHECK(typeNodeP, "geo-location::type", nodeP);
      STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object must be a JSON String");
      EMPTY_STRING_CHECK(nodeP, "the 'type' field of a GeoJSON object cannot be an empty string");

      if (pcheckGeoType(typeNodeP->value.s, &geoType, &details) == false)
      {
        LM_W(("Bad Input (invalid type for geoJson: '%s')", typeNodeP->value.s));
        orionldErrorResponseCreate(OrionldBadRequestData, details, typeNodeP->value.s);
        return false;
      }
      geoTypeString = typeNodeP->value.s;
    }
    else if (SCOMPARE12(nodeP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(coordinatesNodeP, "geo-location::coordinates", nodeP);
      // Point, LineString, Polygon ... in the first level they're all Arrays
      ARRAY_CHECK(coordinatesNodeP, "the 'coordinates' field of a GeoJSON object must be a JSON Array");
    }
    else
    {
      LM_W(("Bad Input (unexpected item in geo-location: '%s')", geoPropertyP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "unexpected item in geo-location", geoPropertyP->name);
      return false;
    }
  }

  if ((typeNodeP == NULL) && (coordinatesNodeP == NULL))
  {
    LM_W(("Bad Input (empty value in geo-location '%s')", geoPropertyP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' and 'type' fields missing for a GeoJSON Property");
    return false;
  }

  if (typeNodeP == NULL)
  {
    LM_W(("Bad Input ('type' missing in geo-location '%s')", geoPropertyP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "Mandatory 'type' field missing for GeoJSON Property",
                               geoPropertyP->name);

    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    LM_W(("Bad Input ('coordinates' missing in geo-location '%s')", geoPropertyP->name));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "The value of an attribute of type GeoProperty must be valid GeoJson",
                               "Mandatory 'coordinates' field missing for a GeoJSON Property");
    return false;
  }

  // Check that the coordinates (coordinatesNodeP) JSON Array coincides with the type 'geoType'
  if (pcheckGeoqCoordinates(coordinatesNodeP, geoType) == false)
    return false;

  //
  // Keep coordsP and typeP for later use (in mongoBackend)
  //
  if (geoTypePP != NULL)
    *geoTypePP   = geoTypeString;
  if (geoCoordsPP != NULL)
    *geoCoordsPP = coordinatesNodeP;

  return true;
}
