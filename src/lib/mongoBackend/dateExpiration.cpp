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
#include "mongoBackend/dateExpiration.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObjBuilder;
using mongo::BSONArrayBuilder;
using orion::CompoundValueNode;



/* ****************************************************************************
*
* getDateExpiration -
*
* Get the ISO8601 Expiration Date information for the given
* ContextAttribute provided as parameter.
*
* It returns true, except in the case of error (in which in addition errDetail gets filled)
*/
static bool getDateExpiration
(
  const ContextAttribute*  caP,
  mongo::Date_t*           dateExpiration,
  std::string*             errDetail
)
{
  if ((caP->type == DATE_TYPE) || (caP->type == DATE_TYPE_ALT))
  {
    *dateExpiration = mongo::Date_t(caP->numberValue);

    return true;
  }

  LM_E(("Runtime Error (attribute detected as date expiration but invalid type: %s)", caP->type.c_str()));
  *errDetail = "error processing date expiration attribute, see log traces";

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
bool processDateExpirationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  mongo::Date_t*                 dateExpiration,
  std::string*                   errDetail,
  OrionError*                    oe
)
{
  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    const ContextAttribute* caP = caV[ix];

    if (caP->name != ENT_EXPIRATION)
    {
      continue;
    }

    if (!getDateExpiration(caP, dateExpiration, errDetail))
    {
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* processDateExpirationAtUpdateAttribute -
*/
//bool processDateExpirationAtUpdateAttribute
//(
//  std::string*                   currentLocAttrName,
//  const ContextAttribute*        targetAttr,
//  mongo::BSONObjBuilder*         geoJson,
//  std::string*                   errDetail,
//  ApiVersion                     apiVersion,
//  OrionError*                    oe
//)
//{
//  std::string subErr;
//
//  //
//  // FIXME P5 https://github.com/telefonicaid/fiware-orion/issues/1142:
//  // note that with the current logic, the name of the attribute meaning location
//  // is preserved on a replace operation. By the moment, we can leave this as it is now
//  // given that the logic in NGSIv2 for specifying location attributes is gogint to change
//  // (the best moment to address this FIXME is probably once NGSIv1 has been deprecated and
//  // removed from the code)
//  //
//  std::string locationString = targetAttr->getLocation(apiVersion);
//
//  /* Check that location (if any) is using the correct coordinates string (it only
//   * makes sense for NGSIv1, this is legacy code that will be eventually removed) */
//  if ((locationString.length() > 0) && (locationString != LOCATION_WGS84) && (locationString != LOCATION_WGS84_LEGACY))
//  {
//    *errDetail = "only WGS84 is supported for location, found: [" + targetAttr->getLocation() + "]";
//    oe->fill(SccBadRequest, *errDetail, "BadRequest");
//    return false;
//  }
//
//  //
//  // Case 1:
//  //   update *to* location. There are 3 sub-cases
//  //
//  if (locationString.length() > 0)
//  {
//    //
//    // Case 1a:
//    //   no location yet -> the updated attribute becomes the location attribute */
//    //
//    if (*currentLocAttrName == "")
//    {
//      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//      {
//        *errDetail = "error parsing location attribute: " + subErr;
//        oe->fill(SccBadRequest, *errDetail, "BadRequest");
//        return false;
//      }
//
//      *currentLocAttrName = targetAttr->name;
//      return true;
//    }
//
//    //
//    // Case 1b:
//    //   currently we have a loation but the attribute holding it is different from the target attribute -> error
//    //
//    if (*currentLocAttrName != targetAttr->name)
//    {
//      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
//                   " when another one has been previously defined [" + *currentLocAttrName + "]";
//
//      oe->fill(SccRequestEntityTooLarge,
//               "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
//               "NoResourcesAvailable");
//
//      return false;
//    }
//
//    //
//    // Case 1c:
//    //   currently we have a location and the attribute holding it is the target attribute -> update the current location
//    //   (note that the shape may change in the process, e.g. geo:point to geo:line)
//    //
//    if (*currentLocAttrName == targetAttr->name)
//    {
//      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//      {
//        *errDetail = "error parsing location attribute: " + subErr;
//        oe->fill(SccBadRequest, *errDetail, "BadRequest");
//        return false;
//      }
//      return true;
//    }
//  }
//
//  //
//  // Case 2:
//  //   update *to* no-location and the attribute previously holding it is the same than the target attribute
//  //   The behaviour is differenet depending on NGSI version
//  //
//  else if (*currentLocAttrName == targetAttr->name)
//  {
//    if (apiVersion == V1)
//    {
//      /* In this case, no-location means that the target attribute doesn't have the "location" metadata. In order
//       * to mantain backwards compabitibility, this is interpreted as a location update */
//      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//      {
//        *errDetail = "error parsing location attribute: " + subErr;
//        oe->fill(SccBadRequest, *errDetail, "BadRequest");
//        return false;
//      }
//    }
//    else  // v2
//    {
//      // Location is null-ified
//      *currentLocAttrName = "";
//    }
//  }
//
//  return true;
//}



/* ****************************************************************************
*
* processLocationAtAppendAttribute -
*/
//bool processDateExpirationAtAppendAttribute
//(
//  std::string*                   currentLocAttrName,
//  const ContextAttribute*        targetAttr,
//  bool                           actualAppend,
//  mongo::BSONObjBuilder*         geoJson,
//  std::string*                   errDetail,
//  ApiVersion                     apiVersion,
//  OrionError*                    oe
//)
//{
//  std::string subErr;
//  std::string locationString = targetAttr->getLocation(apiVersion);
//
//  /* Check that location (if any) is using the correct coordinates string (it only
//     * makes sense for NGSIv1, this is legacy code that will be eventually removed) */
//  if ((locationString.length() > 0) && (locationString != LOCATION_WGS84) && (locationString != LOCATION_WGS84_LEGACY))
//  {
//    *errDetail = "only WGS84 is supported for location, found: [" + targetAttr->getLocation() + "]";
//    oe->fill(SccBadRequest, *errDetail, "BadRequest");
//    return false;
//  }
//
//  /* Case 1: append of new location attribute */
//  if (actualAppend && (locationString.length() > 0))
//  {
//    /* Case 1a: there is a previous location attribute -> error */
//    if (currentLocAttrName->length() != 0)
//    {
//      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
//                   " when another one has been previously defined [" + *currentLocAttrName + "]";
//
//      oe->fill(SccRequestEntityTooLarge,
//               "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
//               "NoResourcesAvailable");
//
//      return false;
//    }
//    /* Case 1b: there isn't any previous location attribute -> new attribute becomes the location attribute */
//    else
//    {
//      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//      {
//        *errDetail = "error parsing location attribute for new attribute: " + subErr;
//        oe->fill(SccBadRequest, *errDetail, "BadRequest");
//        return false;
//      }
//      *currentLocAttrName = targetAttr->name;
//    }
//  }
//  /* Case 2: append-as-update changing attribute type from no-location -> location */
//  else if (!actualAppend && (locationString.length() > 0))
//  {
//    /* Case 2a: there is a previous (which different name) location attribute -> error */
//    if (*currentLocAttrName != targetAttr->name)
//    {
//      *errDetail = "attempt to define a geo location attribute [" + targetAttr->name + "]" +
//                   " when another one has been previously defined [" + *currentLocAttrName + "]";
//
//      oe->fill(SccRequestEntityTooLarge,
//               "You cannot use more than one geo location attribute when creating an entity [see Orion user manual]",
//               "NoResourcesAvailable");
//
//      return false;
//    }
//
//    /* Case 2b: there isn't any previous location attribute -> the updated attribute becomes the location attribute */
//    if (*currentLocAttrName == "")
//    {
//      if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//      {
//        *errDetail = "error parsing location attribute for existing attribute: " + subErr;
//        oe->fill(SccBadRequest, *errDetail, "BadRequest");
//        return false;
//      }
//      *currentLocAttrName = targetAttr->name;
//    }
//
//    /* Case 2c: all pre-conditions ok -> update location with the new value */
//    if (!getGeoJson(targetAttr, geoJson, &subErr, apiVersion))
//    {
//      *errDetail = "error parsing location attribute: " + subErr;
//      oe->fill(SccBadRequest, *errDetail, "BadRequest");
//      return false;
//    }
//    return true;
//  }
//  /* Check 3: in the case of append-as-update, type changes from location -> no-location for the current location
//   * attribute, then remove location attribute */
//  else if (!actualAppend && (locationString.length() == 0) && (*currentLocAttrName == targetAttr->name))
//  {
//    *currentLocAttrName = "";
//  }
//
//  return true;
//}
