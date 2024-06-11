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
#include "common/errorMessages.h"
#include "common/statistics.h"
#include "logMsg/logMsg.h"
#include "ngsi/ContextAttribute.h"
#include "parse/CompoundValueNode.h"
#include "rest/OrionError.h"

// FIXME P5: the following could be not necessary if we optimize the valueBson() thing. See
// the next FIXME P5 comment in this file
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONArrayBuilder.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/location.h"
#include "mongoBackend/compoundValueBson.h"



/* ****************************************************************************
*
* USING
*/
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
* getGeometryFromFeature -
*/
static orion::CompoundValueNode* getGeometryFromFeature(orion::CompoundValueNode* feature)
{
  for (unsigned int ix = 0; ix < feature->childV.size(); ++ix)
  {
    orion::CompoundValueNode* childP = feature->childV[ix];
    if (childP->name == "geometry")
    {
      return childP;
    }
  }

  LM_E(("Runtime Error (geometry field expected in GeoJson Feature)"));
  return NULL;
}



/* ****************************************************************************
*
* getGeometryFromFeatureCollection -
*/
static orion::CompoundValueNode* getGeometryFromFeatureCollection(orion::CompoundValueNode* featureCollection)
{
  for (unsigned int ix = 0; ix < featureCollection->childV.size(); ++ix)
  {
    orion::CompoundValueNode* childP = featureCollection->childV[ix];
    if (childP->name == "features")
    {
      return getGeometryFromFeature(featureCollection->childV[ix]->childV[0]);
    }
  }

  LM_E(("Runtime Error (feature field expected in GeoJson FeatureCollection)"));
  return NULL;
}



/* ****************************************************************************
*
* getGeometry -
*
* Get geometry compound value from attribute, taking into account special GeoJSON
* types such as Feature and FeatureCollection
*/
orion::CompoundValueNode* getGeometry(orion::CompoundValueNode* compoundValueP)
{
  for (unsigned int ix = 0; ix < compoundValueP->childV.size(); ++ix)
  {
     orion::CompoundValueNode* childP = compoundValueP->childV[ix];
     if ((childP->name == "type") && (childP->valueType == orion::ValueTypeString))
     {
       if (childP->stringValue == "Feature")
       {
         return getGeometryFromFeature(compoundValueP);
       }
       if (childP->stringValue == "FeatureCollection")
       {
         return getGeometryFromFeatureCollection(compoundValueP);
       }
     }
  }

  // Regular geo:json
  return compoundValueP;
}



/* ****************************************************************************
*
* isFeatureType -
*
* GeoJSON Feature has an especial treatment. The geometry is extracted from
* "geometry" field at the first level.
*
* Checked:
* - geometry field exists and it's an object
*/
static void isFeatureType(CompoundValueNode* feature, orion::BSONObjBuilder* geoJson, ApiVersion apiVersion, std::string* errP)
{
  for (unsigned int ix = 0; ix < feature->childV.size(); ++ix)
  {
    CompoundValueNode* childP = feature->childV[ix];
    if (childP->name == "geometry")
    {
      if (childP->valueType != orion::ValueTypeObject)
      {
        *errP = "geometry in Feature is not an object";
        return;
      }

      compoundValueBson(childP->childV, *geoJson, apiVersion == V1);
      return;
    }
  }

  *errP = "geometry in Feature not found";
}



/* ****************************************************************************
*
* isFeatureCollectionType -
*
* GeoJSON FeatureCollection has an especial treatment. The geometry is extracted from
* "geometry" field at the first level of the first Feature in the vector. If more
* than one Feature exists an error will be returned (and Orion doesn't know which
* element in the vector has to be used to set the entity location), but not in this
* point, but at parsing stage.
*
* Checked:
*   * the feature field exists
*   * the feature field is an array with exactly one item
*   * the feature field item has a geometry field and it's an object
*/
static void isFeatureCollectionType(CompoundValueNode* featureCollection, orion::BSONObjBuilder* geoJson, ApiVersion apiVersion, std::string* errP)
{
  for (unsigned int ix = 0; ix < featureCollection->childV.size(); ++ix)
  {
    CompoundValueNode* childP = featureCollection->childV[ix];
    if (childP->name == "features")
    {
      if (childP->valueType != orion::ValueTypeVector)
      {
        *errP = "features in FeatureCollection is not an array";
        return;
      }
      else if (childP->childV.size() == 0)
      {
        *errP = "features in FeatureCollection has 0 items";
        return;
      }
      else if (childP->childV.size() > 1)
      {
        *errP = "features in FeatureCollection has more than 1 item";
        return;
      }
      else
      {
        isFeatureType(featureCollection->childV[ix]->childV[0], geoJson, apiVersion, errP);
        return;
      }
    }
  }

  *errP = "features field not found in FeatureCollection";
}


/* ****************************************************************************
*
* isSpecialGeoJsonType -
*
* Return true if an special GeoJSON type was found. In this case, the errP may containt
* an error situation (if errP is empty, then no error occurs).
*
* Return false if no special GeoJSON type was found
*/
static bool isSpecialGeoJsonType(const ContextAttribute* caP, orion::BSONObjBuilder* geoJson, ApiVersion apiVersion, std::string* errP)
{
  *errP = "";

  if (caP->compoundValueP == NULL)
  {
    // This is the case when geo location attribute has null value
    return false;
  }

  for (unsigned int ix = 0; ix < caP->compoundValueP->childV.size(); ++ix)
  {
     CompoundValueNode* childP = caP->compoundValueP->childV[ix];
     if ((childP->name == "type") && (childP->valueType == orion::ValueTypeString))
     {
       if (childP->stringValue == "Feature")
       {
         isFeatureType(caP->compoundValueP, geoJson, apiVersion, errP);
         return true;
       }
       if (childP->stringValue == "FeatureCollection")
       {
         isFeatureCollectionType(caP->compoundValueP, geoJson, apiVersion, errP);
         return true;
       }
     }
  }

  return false;
}


/* ****************************************************************************
*
* getGeoJson -
*
* Get the GeoJSON information (geoJson output argument) for the given
* ContextAttribute provided as parameter.
*
* It returns true, except in the case of error (in which in addition errDetail gets filled)
*
* FIXME P6: try to avoid apiVersion
*
* FIXME P6: review the cases in which this function returns false. Maybe many cases (or all them)
* can be moved to checkGeoJson() in the parsing layer, as preconditions.
*/
static bool getGeoJson
(
  const ContextAttribute*  caP,
  orion::BSONObjBuilder*   geoJson,
  std::string*             errDetail,
  ApiVersion               apiVersion
)
{
  std::vector<double>      coordLat;
  std::vector<double>      coordLong;
  orion::BSONArrayBuilder  ba;

  if ((caP->type == GEO_POINT) || (caP->type == GEO_LINE) || (caP->type == GEO_BOX) || (caP->type == GEO_POLYGON))
  {
    __sync_fetch_and_add(&noOfDprGeoformat, 1);
    if (logDeprecate)
    {
      LM_W(("Deprecated usage of %s detected in attribute %s at entity update, please use geo:json instead", caP->type.c_str(), caP->name.c_str()));
    }
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

    orion::BSONArrayBuilder ba;
    ba.append(aLong);
    ba.append(aLat);
    geoJson->append("coordinates", ba.arr());

    return true;
  }

  if (caP->type == GEO_JSON)
  {
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
    orion::BSONObjBuilder bo;

    // Feature and FeatureCollection has an special treatment, done insise isSpecialGeoJsonType()
    // For other cases (i.e. when isSpecialGeoJsonType() returns false) do it in the "old way"
    if (isSpecialGeoJsonType(caP, geoJson, apiVersion, errDetail))
    {
      // Feature or FeatureCollection was found, but some error may happen
      if (!errDetail->empty())
      {
        return false;
      }
    }
    else
    {
      // Autocast doesn't make sense in this context, strings2numbers enabled in the case of NGSIv1
      // FIXME P7: boolean return value should be managed?
      caP->valueBson(std::string(ENT_ATTRS_VALUE), &bo, "", true, apiVersion == V1);
      geoJson->appendElements(getObjectFieldF(bo.obj(), ENT_ATTRS_VALUE));
    }

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

    orion::BSONArrayBuilder ba1;
    orion::BSONArrayBuilder ba2;
    orion::BSONArrayBuilder ba3;
    orion::BSONArrayBuilder ba4;
    orion::BSONArrayBuilder ba5;

    ba1.append(minLon);
    ba1.append(minLat);

    ba2.append(minLon);
    ba2.append(maxLat);

    ba3.append(maxLon);
    ba3.append(maxLat);

    ba4.append(maxLon);
    ba4.append(minLat);

    ba5.append(minLon);
    ba5.append(minLat);

    ba.append(ba1.arr());
    ba.append(ba2.arr());
    ba.append(ba3.arr());
    ba.append(ba4.arr());
    ba.append(ba5.arr());

    geoJson->append("type", "Polygon");
    orion::BSONArrayBuilder baContainer;
    baContainer.append(ba.arr());
    geoJson->append("coordinates", baContainer.arr());

    return true;
  }

  // geo:line and geo:polygon (different from geo:box) both build the coords array in the same way
  for (unsigned int ix = 0; ix < coordLat.size(); ix++)
  {
    orion::BSONArrayBuilder ba1;
    ba1.append(coordLong[ix]);
    ba1.append(coordLat[ix]);

    ba.append(ba1.arr());
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
    orion::BSONArrayBuilder baContainer;
    baContainer.append(ba.arr());
    geoJson->append("coordinates", baContainer.arr());

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
  orion::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  *locAttr = "";

  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    const ContextAttribute* caP = caV[ix];

    if (!caP->getLocation(NULL))
    {
      continue;
    }

    if (!locAttr->empty())
    {
      *errDetail = ERROR_DESC_NO_RESOURCES_AVAILABLE_GEOLOC;
      oe->fill(SccRequestEntityTooLarge, *errDetail, ERROR_NO_RESOURCES_AVAILABLE);
      return false;
    }

    if (!getGeoJson(caP, geoJson, errDetail, apiVersion))
    {
      oe->fill(SccBadRequest, "error parsing location attribute: " + *errDetail, ERROR_BAD_REQUEST);
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
  orion::BSONObj*                attrsP,
  const ContextAttribute*        targetAttr,
  orion::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  std::string subErr;

  //
  // Case 1:
  //   update *to* location. There are 3 sub-cases
  //
  if (targetAttr->getLocation(attrsP))
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
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
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

        oe->fill(SccRequestEntityTooLarge, ERROR_DESC_NO_RESOURCES_AVAILABLE_GEOLOC, ERROR_NO_RESOURCES_AVAILABLE);

        return false;
      }
      else
      {
        if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
        {
          *errDetail = "error parsing location attribute: " + subErr;
          oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
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
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
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
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
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
  orion::BSONObj*                attrsP,
  const ContextAttribute*        targetAttr,
  bool                           actualAppend,
  orion::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
)
{
  std::string subErr;
  bool        isALocation = targetAttr->getLocation(attrsP);

  /* Case 1: append of new location attribute */
  if (actualAppend && isALocation)
  {
    /* Case 1a: there is a previous location attribute -> error */
    if (!currentLocAttrName->empty())
    {
      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
                   " when another one has been previously defined [" + *currentLocAttrName + "]";

      oe->fill(SccRequestEntityTooLarge, ERROR_DESC_NO_RESOURCES_AVAILABLE_GEOLOC, ERROR_NO_RESOURCES_AVAILABLE);

      return false;
    }
    /* Case 1b: there isn't any previous location attribute -> new attribute becomes the location attribute */
    else
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute for new attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
        return false;
      }
      *currentLocAttrName = targetAttr->name;
    }
  }
  /* Case 2: append-as-update changing attribute type from no-location -> location */
  else if (!actualAppend && isALocation)
  {
    /* Case 2a: there is a previous (not empty and with different name) location attribute -> error */
    if ((!currentLocAttrName->empty()) && (*currentLocAttrName != targetAttr->name))
    {
      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
                   " when another one has been previously defined [" + *currentLocAttrName + "]";

      oe->fill(SccRequestEntityTooLarge, ERROR_DESC_NO_RESOURCES_AVAILABLE_GEOLOC, ERROR_NO_RESOURCES_AVAILABLE);

      return false;
    }

    /* Case 2b: there isn't any previous location attribute -> the updated attribute becomes the location attribute */
    if (*currentLocAttrName == "")
    {
      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
      {
        *errDetail = "error parsing location attribute for existing attribute: " + subErr;
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
        return false;
      }
      *currentLocAttrName = targetAttr->name;
    }

    /* Case 2c: all pre-conditions ok -> update location with the new value */
    if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
    {
      *errDetail = "error parsing location attribute: " + subErr;
      oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
      return false;
    }
    return true;
  }
  /* Check 3: in the case of append-as-update, type changes from location -> no-location for the current location
   * attribute, then remove location attribute */
  else if (!actualAppend && !isALocation && (*currentLocAttrName == targetAttr->name))
  {
    *currentLocAttrName = "";
  }

  return true;
}
