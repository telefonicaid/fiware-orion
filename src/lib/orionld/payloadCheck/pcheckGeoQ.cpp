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
#include <string.h>                                             // strcmp
extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjRenderSize.h"                                 // kjFastRenderSize
#include "kjson/kjRender.h"                                     // kjFastRender
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/dotForEq.h"                            // dotForEq
#include "orionld/types/OrionldProblemDetails.h"                // OrionldProblemDetails
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/payloadCheck/PCHECK.h"                        // PCHECK_*
#include "orionld/payloadCheck/pcheckGeoType.h"                 // pcheckGeoType
#include "orionld/payloadCheck/pcheckGeoqCoordinates.h"         // pcheckGeoqCoordinates
#include "orionld/payloadCheck/pcheckGeoqGeorel.h"              // pcheckGeoqGeorel
#include "orionld/payloadCheck/pcheckGeoQ.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// ngsildCoordinatesToAPIv1Datamodel -
//
bool ngsildCoordinatesToAPIv1Datamodel(KjNode* coordinatesP, const char* fieldName, KjNode* geometryP)
{
  bool   isPoint = false;
  char*  buf;

  if (geometryP == NULL)
  {
    orionldError(OrionldBadRequestData, "Internal Error", "Unable to extract the geometry of a geoQ for coordinmate APIv1 fix", 400);
    return false;
  }

  // Must be called "coords" in the database
  coordinatesP->name = (char*) "coords";

  // If already a String, then we're done
  if (coordinatesP->type == KjString)
    return true;

  if (strcmp(geometryP->value.s, "Point") == 0)
    isPoint = true;

  if (isPoint)
  {
    // A point is an array ( [ 1, 2 ] ) in NGSI-LD, but in APIv1 database mode it is a string ( "1,2" )
    int    coords = 0;
    float  coordV[3];

    for (KjNode* coordP = coordinatesP->value.firstChildP; coordP != NULL; coordP = coordP->next)
    {
      if (coordP->type == KjFloat)
        coordV[coords] = coordP->value.f;
      else
        coordV[coords] = (float) coordP->value.i;

      ++coords;
    }

    int bufSize = 128;
    buf = kaAlloc(&orionldState.kalloc, bufSize);
    if (coords == 2)
      snprintf(buf, bufSize, "%f,%f", coordV[0], coordV[1]);
    else
      snprintf(buf, bufSize, "%f,%f,%f", coordV[0], coordV[1], coordV[2]);
  }
  else
  {
    int bufSize = kjFastRenderSize(coordinatesP);
    buf = kaAlloc(&orionldState.kalloc, bufSize);
    kjFastRender(coordinatesP, buf);
  }

  coordinatesP->type    = KjString;
  coordinatesP->value.s = buf;

  return true;
}



// ----------------------------------------------------------------------------
//
// pcheckGeoQ -
//
bool pcheckGeoQ(KjNode* geoqNodeP, bool coordsToString)
{
  KjNode*                geometryP    = NULL;
  KjNode*                coordinatesP = NULL;
  KjNode*                georelP      = NULL;
  KjNode*                geopropertyP = NULL;
  OrionldGeoJsonType     geoJsonType;
  OrionldProblemDetails  pd;

  for (KjNode* itemP = geoqNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(itemP->name, "geometry") == 0)
    {
      DUPLICATE_CHECK(geometryP, "geometry", itemP);
      STRING_CHECK(geometryP, "the 'geometry' field of a GeoJSON object must be a JSON String");
    }
    else if (strcmp(itemP->name, "coordinates") == 0)
    {
      DUPLICATE_CHECK(coordinatesP, "coordinates", itemP);
      PCHECK_STRING_OR_ARRAY(coordinatesP, 0, "Invalid Data Type", "the 'coordinates' field of a GeoJSON object must be a JSON Array (or String)", 400);
    }
    else if (strcmp(itemP->name, "georel") == 0)
    {
      DUPLICATE_CHECK(georelP, "georel", itemP);
      STRING_CHECK(georelP, "the 'georel' field of a GeoJSON object must be a JSON String");
    }
    else if (strcmp(itemP->name, "geoproperty") == 0)
    {
      DUPLICATE_CHECK(geopropertyP, "geoproperty", itemP);
      STRING_CHECK(georelP, "the 'geoproperty' field of a GeoJSON object must be a JSON String");
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid Payload Data - invalid field in geoQ", itemP->name, 400);
      return false;
    }
  }

  if (geometryP == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Payload Data", "mandatory geoQ field 'geometry' is missing", 400);
    return false;
  }

  if (coordinatesP == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Payload Data", "mandatory geoQ field 'coordinates' is missing", 400);
    return false;
  }

  if (georelP == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Payload Data", "mandatory geoQ field 'georel' is missing", 400);
    return false;
  }

  char* detail;
  if (pcheckGeoType(geometryP->value.s, &geoJsonType, &detail) == false)  // Rename to pcheckGeoqGeometry
  {
    orionldError(OrionldBadRequestData, "Invalid Payload Data", detail, 400);
    return false;
  }

  if (pcheckGeoqCoordinates(coordinatesP, geoJsonType) == false)
  {
    orionldError(OrionldBadRequestData, "Invalid Payload Data", "invalid coordinates", 400);
    return false;
  }

  if (pcheckGeoqGeorel(georelP, geoJsonType, &pd) == false)
  {
    orionldError(OrionldBadRequestData, pd.title, pd.detail, 400);
    return false;
  }

  //
  // If geoproperty has been given, the name must be expanded, and any dots replaced by '='
  //
  if (geopropertyP != NULL)
  {
    char* pName = geopropertyP->value.s;

    if (strcmp(pName, "location") != 0)
    {
      geopropertyP->value.s = orionldAttributeExpand(orionldState.contextP, pName, true, NULL);
      dotForEq(geopropertyP->value.s);
    }
  }

  //
  // Render the coordinates and convert to a string - for the NGSIv1 database model
  //
  if (coordsToString == true)
  {
    if (ngsildCoordinatesToAPIv1Datamodel(coordinatesP, "geoQ::coordinates", geometryP) == false)
      return false;
  }

  return true;
}
