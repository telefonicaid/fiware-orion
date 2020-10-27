/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Fermin Galan
*
*/
#include <string>
#include <vector>

#include "common/string.h"
#include "common/globals.h"
#include "logMsg/logMsg.h"
#include "ngsi/ContextAttribute.h"
#include "parse/CompoundValueNode.h"
#include "rest/OrionError.h"

// FIXME P5: the following could be not necessary if we optimize the valueBson() thing. See
// the next FIXME P5 comment in this file
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/location.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObjBuilder;
using mongo::BSONArrayBuilder;
using orion::CompoundValueNode;



/* ****************************************************************************
*
* stringArray2coords -
*/
static bool stringArray2coords
(
  const ContextAttribute*  caP,
  std::vector<double>*     coordLat,
  std::vector<double>*     coordLong,
  std::string*             errDetail
)
{
  if ((caP->compoundValueP == NULL) || !(caP->compoundValueP->isVector()))
  {
    *errDetail = "geo:line, geo:box and geo:polygon needs array of strings as value";
    return false;
  }

  for (unsigned int ix = 0; ix < caP->compoundValueP->childV.size() ; ix++)
  {
    CompoundValueNode* item = caP->compoundValueP->childV[ix];

    if (!item->isString())
    {
      *errDetail = "geo:line, geo:box and geo:polygon needs array of strings but some element is not a string";
      return false;
    }

    double lat;
    double lon;

    if (!string2coords(item->stringValue, lat, lon))
    {
      *errDetail = "geo coordinates format error [see Orion user manual]: " + item->stringValue;
      return false;
    }

    coordLat->push_back(lat);
    coordLong->push_back(lon);
  }

  return true;
}



/* ****************************************************************************
*
* getGeoJson -
*
* Get the GeoJSON information (geoJson output argument) for the given
* ContextAttribute provided as parameter.
*
* It returns true, except in the case of error (in which in addition errDetail gets filled)
*/
static bool getGeoJson
(
  const ContextAttribute*  caP,
  BSONObjBuilder*          geoJson,
  std::string*             errDetail,
  ApiVersion               apiVersion
)
{
  std::vector<double>  coordLat;
  std::vector<double>  coordLong;
  BSONArrayBuilder     ba;

  if ((apiVersion == V1) && (caP->type != GEO_POINT) && (caP->type != GEO_LINE) && (caP->type != GEO_BOX) &&
      (caP->type != GEO_POLYGON) && (caP->type != GEO_JSON))
  {
    // This corresponds to the legacy way in NGSIv1 based in metadata
    // The block is the same that for GEO_POINT but it is clearer if we keep it separated

    double  aLat;
    double  aLong;

    if (!string2coords(caP->stringValue, aLat, aLong))
    {
      *errDetail = "geo coordinates format error [see Orion user manual]: " + caP->stringValue;
      return false;
    }

    geoJson->append("type", "Point");
    geoJson->append("coordinates", BSON_ARRAY(aLong << aLat));

    return true;
  }

  if (caP->type == GEO_POINT)
  {
    double  aLat;
    double  aLong;

    if (!string2coords(caP->stringValue, aLat, aLong))
    {
      *errDetail = "geo coordinates format error [see Orion user manual]: " + caP->stringValue;
      return false;
    }

    geoJson->append("type", "Point");
    geoJson->append("coordinates", BSON_ARRAY(aLong << aLat));

    return true;
  }

  if (caP->type == GEO_JSON)
  {
    // Attribute value has to be object in this case
    if ((caP->compoundValueP == NULL) || !(caP->compoundValueP->isObject()))
    {
      *errDetail = "geo:json needs an object as value";
      return false;
    }

    /* FIXME P5: the procedure we are currently using is sub-optimal, as valueBson() is
     * called from location.cpp one time to generate the BSON for the location.coords field
     * and called again (before or after... I'm not sure right now and may depend on the
     * particular opartion, i.e. entity creation, update APPEND or update UPPDATE) to
     * generated the BSON for the attribute value. We should find a way of calling just one
     * time.
     *
     * Note that valueBson() puts the BSON object in the "value" field of the builder passed
     * as argument.
     */
    BSONObjBuilder bo;

    // Autocast doesn't make sense in this context, strings2numbers enabled in the case of NGSIv1
    caP->valueBson(bo, "", true, apiVersion == V1);
    geoJson->appendElements(getObjectFieldF(bo.obj(), ENT_ATTRS_VALUE));

    return true;
  }

  // geo:line, geo:box and geo:polygon use vector of coordinates
  if (!stringArray2coords(caP, &coordLat, &coordLong, errDetail))
  {
    return false;
  }

  // geo:box is a bit special, it composed "directly" the coords array based on opposite corners
  if (caP->type == GEO_BOX)
  {
    // Exactly 2 points (just checking one of the vectors is enough)
    if (coordLat.size() != 2)
    {
      *errDetail = "geo:box uses exactly 2 coordinates";
      return false;
    }

    double minLat;
    double minLon;
    double maxLat;
    double maxLon;

    if (!orderCoordsForBox(&minLat, &maxLat, &minLon, &maxLon, coordLat[0], coordLat[1], coordLong[0], coordLong[1]))
    {
      *errDetail = "geo:box coordinates are not defining an actual box";
      return false;
    }

    ba.append(BSON_ARRAY(minLon << minLat));
    ba.append(BSON_ARRAY(minLon << maxLat));
    ba.append(BSON_ARRAY(maxLon << maxLat));
    ba.append(BSON_ARRAY(maxLon << minLat));
    ba.append(BSON_ARRAY(minLon << minLat));

    geoJson->append("type", "Polygon");
    geoJson->append("coordinates", BSON_ARRAY(ba.arr()));

    return true;
  }

  // geo:line and geo:polygon (different from geo:box) both build the coords array in the same way
  for (unsigned int ix = 0; ix < coordLat.size(); ix++)
  {
    ba.append(BSON_ARRAY(coordLong[ix] << coordLat[ix]));
  }

  if (caP->type == GEO_LINE)
  {
    // At least 2 points (just checking one of the vectors is enough)
    if (coordLat.size() < 2)
    {
      *errDetail = "geo:line uses at least 2 coordinates";
      return false;
    }

    geoJson->append("type", "LineString");
    geoJson->append("coordinates", ba.arr());

    return true;
  }

  if (caP->type == GEO_POLYGON)
  {
    // At least 4 points (just checking one of the vectors is enough)
    if (coordLat.size() < 4)
    {
      *errDetail = "geo:polygon uses at least 4 coordinates";
      return false;
    }

    // First and last coordinates must be the same
    int n = coordLat.size();

    if ((coordLat[0] != coordLat[n-1]) || (coordLong[0] != coordLong[n-1]))
    {
      *errDetail = "geo:polygon first and last coordinates must match";
      return false;
    }

    geoJson->append("type", "Polygon");
    geoJson->append("coordinates", BSON_ARRAY(ba.arr()));

    return true;
  }

  LM_E(("Runtime Error (attribute detected as location but unknown type: %s)", caP->type.c_str()));
  *errDetail = "error processing geo location attribute, see log traces";

  return false;
}



/* ****************************************************************************
*
* processLocationAtEntityCreation -
*
* This function process the context attribute vector, searching for an attribute meaning
* location. In that case, it fills geoJson. If a location attribute is not found, then
* locAttr is filled with an empty string, i.e. "".
*
* This function always return true (no matter if the attribute was found or not), except in an
* error situation, in which case errorDetail is filled.
*/
bool processLocationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  std::string*                   locAttr,
  BSONObjBuilder*                geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  *locAttr = "";

  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    const ContextAttribute* caP = caV[ix];

    std::string location = caP->getLocation(apiVersion);

    if (location.empty())
    {
      continue;
    }

    if (!locAttr->empty())
    {
      *errDetail = "You cannot use more than one geo location attribute "
                   "when creating an entity [see Orion user manual]";
      oe->fill(SccRequestEntityTooLarge, *errDetail, "NoResourcesAvailable");
      return false;
    }

    if ((location != LOCATION_WGS84) && (location != LOCATION_WGS84_LEGACY))
    {
      *errDetail = "only WGS84 are supported, found: " + location;
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }

    if (!getGeoJson(caP, geoJson, errDetail, apiVersion))
    {
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }

    *locAttr = caP->name;
  }

  return true;
}



/* ****************************************************************************
*
* processLocationAtUpdateAttribute -
*/
bool processLocationAtUpdateAttribute
(
  std::string*                   currentLocAttrName,
  const ContextAttribute*        targetAttr,
  mongo::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  std::string subErr;
  std::string locationString = targetAttr->getLocation(apiVersion);

  /* Check that location (if any) is using the correct coordinates string (it only
   * makes sense for NGSIv1, this is legacy code that will be eventually removed) */
  if ((!locationString.empty()) && (locationString != LOCATION_WGS84) && (locationString != LOCATION_WGS84_LEGACY))
  {
    *errDetail = "only WGS84 is supported for location, found: [" + targetAttr->getLocation() + "]";
    oe->fill(SccBadRequest, *errDetail, "BadRequest");
    return false;
  }

  //
  // Case 1:
  //   update *to* location. There are 3 sub-cases
  //
  if (!locationString.empty())
  {
    //
    // Case 1a:
    //   no location yet -> the updated attribute becomes the location attribute */
    //
    if (*currentLocAttrName == "")
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, "BadRequest");
        return false;
      }

      *currentLocAttrName = targetAttr->name;
      return true;
    }

    //
    // Case 1b:
    //   currently we have a loation but the attribute holding it is different from the target attribute.
    //   The behaviour is different depending on NGSI version
    //
    if (*currentLocAttrName != targetAttr->name)
    {
      if (apiVersion == V1)
      {
        *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
                     " when another one has been previously defined [" + *currentLocAttrName + "]";

        oe->fill(SccRequestEntityTooLarge,
                 "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
                 "NoResourcesAvailable");

        return false;
      }
      else
      {
        if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
        {
          *errDetail = "error parsing location attribute: " + subErr;
          oe->fill(SccBadRequest, *errDetail, "BadRequest");
          return false;
        }
        *currentLocAttrName = targetAttr->name;
        return true;
      }
    }
    //
    // Case 1c:
    //   currently we have a location and the attribute holding it is the target attribute -> update the current location
    //   (note that the shape may change in the process, e.g. geo:point to geo:line)
    //
    if (*currentLocAttrName == targetAttr->name)
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, "BadRequest");
        return false;
      }
      return true;
    }
  }

  //
  // Case 2:
  //   update *to* no-location and the attribute previously holding it is the same than the target attribute
  //   The behaviour is different depending on NGSI version
  //
  else if (*currentLocAttrName == targetAttr->name)
  {
    if (apiVersion == V1)
    {
      /* In this case, no-location means that the target attribute doesn't have the "location" metadata. In order
       * to mantain backwards compabitibility, this is interpreted as a location update */
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, "BadRequest");
        return false;
      }
    }
    else  // v2
    {
      // Location is null-ified
      *currentLocAttrName = "";
    }
  }

  return true;
}



/* ****************************************************************************
*
* processLocationAtAppendAttribute -
*/
bool processLocationAtAppendAttribute
(
  std::string*                   currentLocAttrName,
  const ContextAttribute*        targetAttr,
  bool                           actualAppend,
  mongo::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  std::string subErr;
  std::string locationString = targetAttr->getLocation(apiVersion);

  /* Check that location (if any) is using the correct coordinates string (it only
     * makes sense for NGSIv1, this is legacy code that will be eventually removed) */
  if ((!locationString.empty()) && (locationString != LOCATION_WGS84) && (locationString != LOCATION_WGS84_LEGACY))
  {
    *errDetail = "only WGS84 is supported for location, found: [" + targetAttr->getLocation() + "]";
    oe->fill(SccBadRequest, *errDetail, "BadRequest");
    return false;
  }

  /* Case 1: append of new location attribute */
  if (actualAppend && (!locationString.empty()))
  {
    /* Case 1a: there is a previous location attribute -> error */
    if (!currentLocAttrName->empty())
    {
      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
                   " when another one has been previously defined [" + *currentLocAttrName + "]";

      oe->fill(SccRequestEntityTooLarge,
               "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
               "NoResourcesAvailable");

      return false;
    }
    /* Case 1b: there isn't any previous location attribute -> new attribute becomes the location attribute */
    else
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute for new attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, "BadRequest");
        return false;
      }
      *currentLocAttrName = targetAttr->name;
    }
  }
  /* Case 2: append-as-update changing attribute type from no-location -> location */
  else if (!actualAppend && (!locationString.empty()))
  {
    /* Case 2a: there is a previous (not empty and which different name) location attribute -> error */
    if ((!currentLocAttrName->empty()) && (*currentLocAttrName != targetAttr->name))
    {
      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
                   " when another one has been previously defined [" + *currentLocAttrName + "]";

      oe->fill(SccRequestEntityTooLarge,
               "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
               "NoResourcesAvailable");

      return false;
    }

    /* Case 2b: there isn't any previous location attribute -> the updated attribute becomes the location attribute */
    if (*currentLocAttrName == "")
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute for existing attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, "BadRequest");
        return false;
      }
      *currentLocAttrName = targetAttr->name;
    }

    /* Case 2c: all pre-conditions ok -> update location with the new value */
    if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
    {
      *errDetail = "error parsing location attribute: " + subErr;
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }
    return true;
  }
  /* Check 3: in the case of append-as-update, type changes from location -> no-location for the current location
   * attribute, then remove location attribute */
  else if (!actualAppend && (locationString.empty()) && (*currentLocAttrName == targetAttr->name))
  {
    *currentLocAttrName = "";
  }

  return true;
}
