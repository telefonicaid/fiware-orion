/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/string.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Scope.h"
#include "parse/forbiddenChars.h"
#include "orionld/common/orionldState.h"

using namespace orion;



/* ****************************************************************************
*
* Scope::Scope -
*/
Scope::Scope()
{
  type     = "";
  value    = "";
  oper     = "";
  areaType = orion::NoArea;

  georel.maxDistance = -1;
  georel.minDistance = -1;

  stringFilterP      = NULL;
  mdStringFilterP    = NULL;
}



/* ****************************************************************************
*
* Scope::Scope -
*/
Scope::Scope(const std::string& _type, const std::string& _value, const std::string& _oper)
{
  type     = _type;
  value    = _value;
  oper     = _oper;
  areaType = orion::NoArea;

  georel.maxDistance = -1;
  georel.minDistance = -1;

  stringFilterP      = NULL;
  mdStringFilterP    = NULL;
}



/* ****************************************************************************
*
* pointVectorRelease -
*/
static void pointVectorRelease(const std::vector<orion::Point*>& pointV)
{
  for (unsigned int ix = 0; ix < pointV.size(); ++ix)
  {
    delete(pointV[ix]);
  }
}



/* ****************************************************************************
*
* Scope::fill -
*/
int Scope::fill
(
  ApiVersion          apiVersion,
  const std::string&  geometryString,
  const std::string&  coordsString,
  const std::string&  georelString,
  std::string*        errorStringP
)
{
  Geometry                    geometry;
  std::vector<std::string>    pointStringV;
  int                         points;
  std::vector<orion::Point*>  pointV;
  std::string                 coordsString2 = coordsString;
  std::string                 georelString2 = georelString;

  type = (apiVersion == V1)? FIWARE_LOCATION : FIWARE_LOCATION_V2;

#ifdef ORIONLD
  //
  // The syntaxis of a polygon in APIv2 is:
  //   40,-3;36,-4;44,-4;40,-3 ...
  //
  // In NGSI-LD it is:
  // [ [40, -3], [36, -4], [44, -4], [40, -3] ... ]
  //
  //
  // This loop eliminates all '[' and ']' and changes the outer commas for ';'
  //
  char convertedCoordsString[512];  // Simply HAS to be enough!

  if ((apiVersion == NGSI_LD_V1) && (geometryString == "Polygon"))
  {
    char* cP    = (char*) coordsString2.c_str();
    char* in    = cP;
    int   ccsIx = 0;

    bzero(convertedCoordsString, sizeof(convertedCoordsString));

    // Skip whitespace
    while (*cP == ' ') ++cP;

    if (*cP != '[')
    {
      // Error
      LM_E(("Geo: Converting NGSI-LD polygon coordinates: not starting with '['"));
      *errorStringP = std::string("Converting NGSI-LD polygon coordinates: not starting with '['");
      return -1;
    }
    ++cP;  // Skipping initial '['

    // Skip whitespace
    while (*cP == ' ') ++cP;

    if (*cP != '[')
    {
      // Error
      LM_E(("Geo: Converting NGSI-LD polygon coordinates: not starting with '[['"));
      *errorStringP = std::string("Converting NGSI-LD polygon coordinates: not starting with '[['");
      return -1;
    }
    ++cP;  // Skipping second '['

    // Skip whitespace
    while (*cP == ' ') ++cP;

    int  coords    = 1;
    bool something = false;

    while (*cP != 0)
    {
      if (*cP != ']')
      {
        // Must be 0..9, a dot, a '-' or a comma

        if ((*cP >= '0') && (*cP <= '9'))
          something = true;
        else if (*cP == ',')
          ++coords;
        else if ((*cP == '.') || (*cP == '-') || (*cP == ' '))
          ;
        else
        {
          LM_E(("Geo: Converting NGSI-LD polygon coordinates: invalid character: '%c' (whole string: %s)", *cP, in));
          *errorStringP = std::string("Converting NGSI-LD polygon coordinates: invalid character");
          return -1;
        }

        convertedCoordsString[ccsIx] = *cP;
        ++ccsIx;
      }
      else  // Must be '],[' or ']]\0'
      {
        ++cP;  // Skip first ']'

        if ((*cP == ',') && (cP[1] == '['))
        {
          // Insert ';' instead of "],["
          ++cP;
          convertedCoordsString[ccsIx] = ';';
          ++ccsIx;
        }
        else if ((*cP == ',') && (cP[1] == ' ') && (cP[2] == '['))
        {
          // Insert ';' instead of "], ["
          cP += 2;
          convertedCoordsString[ccsIx] = ';';
          ++ccsIx;
        }
        else if ((*cP == ']') && (cP[1] == 0))
        {
          // Done
          break;
        }
        else if ((*cP == ' ') && (cP[1] == ']') && (cP[2] == 0))
        {
          // Done
          break;
        }
        else if (*cP == 0)  // ???
        {
          // Done - must break here to not advance cP
          break;
        }
        else  // Error
        {
          LM_E(("Geo: Invalid polygon: '%s' (at: '%s')", in, cP));
          *errorStringP = std::string("Invalid polygon");
          return -1;
        }
      }

      ++cP;
    }

    if (something == false)
    {
      LM_E(("Garbage coordinates URI param?"));
      *errorStringP = std::string("Garbage coordinates URI param?");
      return -1;
    }
    else if (coords == 1)
    {
      LM_E(("At least TWO coordinates must be present"));
      *errorStringP = std::string("At least TWO coordinates must be present");
      return -1;
    }

    convertedCoordsString[ccsIx] = 0;
    coordsString2 = convertedCoordsString;

    if (georelString == "within")
      georelString2 = "coveredBy";
  }
#endif


  //
  // parse geometry
  //
  std::string errorString;
  if (geometry.parse(apiVersion, geometryString.c_str(), &errorString) != 0)
  {
    *errorStringP = std::string("error parsing geometry: ") + errorString;
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }


  //
  // Parse georel?
  //
  if (georelString2 != "")
  {
    if (georel.parse(georelString2.c_str(), errorStringP) != 0)
    {
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      return -1;
    }
  }

  // Check invalid combinations
  if ((geometry.areaType == "line") && (georel.type == "coveredBy"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue $within not supported with provided geometry:
     *    { $geoWithin: { $geometry: { type: "LineString", coordinates: [ [ 5.0...", code: 17287 }
     */

    *errorStringP = "line geometry cannot be used with coveredBy georel";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }

  if ((geometry.areaType == "point") && (georel.type == "coveredBy"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue $within not supported with provided geometry:
     *    { $geoWithin: { $geometry: { type: "Point", coordinates: [ [ 5.0...", code: 17287 }
     */

    *errorStringP = "point geometry cannot be used with coveredBy georel";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }

  if ((geometry.areaType != "point") && (georel.type == "near"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue invalid point in geo near query $geometry argument:
     *   { type: "Polygon", coordinates: [ [ [ 2.0, 1.0 ], [ 4.0, 3.0 ],...", code: 17287 }
     */

    *errorStringP = "georel /near/ used with geometry different than point";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }

  //
  // Split coordsString into a vector of points, or pairs of coordinates
  //
  if (coordsString2 == "")
  {
    *errorStringP = "no coordinates for geometry";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }

  points = stringSplit(coordsString2, ';', pointStringV);
  if (points == 0)
  {
    *errorStringP = "erroneous coordinates for geometry";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));
    return -1;
  }

  //
  // Convert point-strings into instances of the orion::Point class
  //
  for (int ix = 0; ix < points; ++ix)
  {
    std::vector<std::string>  coordV;
    int                       coords;
    double                    latitude;
    double                    longitude;

    coords = stringSplit(pointStringV[ix], ',', coordV);

    if (geometry.areaType == "point")
    {
      //
      // NGSIv2 only allows for 2 coords (no altitude) while NGSI-LD allows for 2 or three
      //
      bool error = false;

      if (orionldState.apiVersion == NGSI_LD_V1)
      {
        if ((coords != 2) && (coords != 3))
          error = true;
      }
      else if (coords != 2)
        error = true;

      if (error == true)
      {
        *errorStringP = "invalid coordinates for point";
        LM_E(("geometry.parse: %s (%d coords)", errorStringP->c_str(), coords));
        pointVectorRelease(pointV);
        pointV.clear();
        return -1;
      }
    }

    if (coords < 2)
    {
      *errorStringP = "invalid point in URI param /coords/";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    if (!str2double(coordV[0].c_str(), &latitude))
    {
      *errorStringP = "invalid coordinates";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    if (!str2double(coordV[1].c_str(), &longitude))
    {
      *errorStringP = "invalid coordinates";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    if (apiVersion == NGSI_LD_V1)  // SWAP
    {
      // Swapping longitude and latitude
      double saved = longitude;
      longitude    = latitude;
      latitude     = saved;
    }

    orion::Point* pointP = new Point(latitude, longitude);
    pointV.push_back(pointP);
  }


  if (geometry.areaType == "circle")
  {
    if (apiVersion == V2)
    {
      *errorStringP = "circle geometry is not supported by Orion API v2";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    else
    {
      if (pointV.size() != 1)
      {
        *errorStringP = "Too many coordinates for circle";
        LM_E(("geometry.parse: %s", errorStringP->c_str()));
        pointVectorRelease(pointV);
        pointV.clear();
        return -1;
      }

      areaType = orion::CircleType;

      circle.radiusSet(geometry.radius);
      circle.invertedSet(geometry.external);
      circle.centerSet(pointV[0]);

      pointVectorRelease(pointV);
      pointV.clear();
    }
  }
  else if (geometry.areaType == "polygon")
  {
    areaType = orion::PolygonType;

    if ((apiVersion == V1) && (pointV.size() < 3))
    {
      *errorStringP = "Too few coordinates for polygon";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    else if ((apiVersion == V2) && (pointV.size() < 4))
    {
      *errorStringP = "Too few coordinates for polygon";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    //
    // If v2, first and last point must be identical
    //
    if ((apiVersion == V2) && (pointV[0]->equals(pointV[pointV.size() - 1]) == false))
    {
      *errorStringP = "First and last point in polygon not the same";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    polygon.invertedSet(geometry.external);

    for (unsigned int ix = 0; ix < pointV.size(); ++ix)
    {
      polygon.vertexAdd(pointV[ix]);
    }
    pointV.clear();
  }
  else if (geometry.areaType == "line")
  {
    areaType = orion::LineType;

    if (pointV.size() < 2)
    {
      *errorStringP = "invalid number of coordinates for /line/";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    for (unsigned int ix = 0; ix < pointV.size(); ++ix)
    {
      line.pointAdd(pointV[ix]);
    }
    pointV.clear();
  }
  else if (geometry.areaType == "box")
  {
    areaType = orion::BoxType;

    if (pointV.size() != 2)
    {
      *errorStringP = "invalid number of coordinates for /box/";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    double minLat;
    double maxLat;
    double minLon;
    double maxLon;
    if (!orderCoordsForBox(&minLat, &maxLat, &minLon, &maxLon, pointV[0]->latitude(), pointV[1]->latitude(), pointV[0]->longitude(), pointV[1]->longitude()))
    {
      *errorStringP = "box coordinates are not defining an actual box";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    // Lower left: smaller lat and long, upper right: greater lat and long
    Point ll;
    ll.latitudeSet(minLat);
    ll.longitudeSet(minLon);
    Point ur;
    ur.latitudeSet(maxLat);
    ur.longitudeSet(maxLon);
    box.fill(&ll, &ur);

    pointVectorRelease(pointV);
    pointV.clear();
  }
  else if (geometry.areaType == "point")
  {
    areaType = orion::PointType;

    if (pointV.size() != 1)
    {
      *errorStringP = "invalid number of coordinates for /point/";
      LM_E(("geometry.parse: %s", errorStringP->c_str()));
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    point.fill(pointV[0]);

    pointVectorRelease(pointV);
    pointV.clear();
  }
  else
  {
    areaType = orion::NoArea;
    *errorStringP = "invalid area-type";
    LM_E(("geometry.parse: %s", errorStringP->c_str()));

    pointVectorRelease(pointV);
    pointV.clear();
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* Scope::render -
*/
std::string Scope::render(bool notLastInVector)
{
  std::string out      = "";
  const char* tTag     = "type";
  const char* vTag     = "value";

  out += startTag();
  out += valueTag(tTag, type, true);
  out += valueTag(vTag, value);
  out += endTag(notLastInVector);

  return out;
}



/* ****************************************************************************
*
* Scope::check -
*/
std::string Scope::check(void)
{
  //
  // Check for forbidden characters
  //
  if (forbiddenChars(type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of a scope");
    return "illegal chars in scope type";
  }

  if ((type != SCOPE_TYPE_SIMPLE_QUERY) && (type != SCOPE_TYPE_SIMPLE_QUERY_MD))
  {
    if (forbiddenChars(value.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of a scope");
      return "illegal chars in scope";
    }
  }

  if (type == FIWARE_LOCATION || type == FIWARE_LOCATION_DEPRECATED)
  {
    if (areaType == orion::CircleType)
    {
      if (circle.radiusString() == "0")
      {
        alarmMgr.badInput(clientIp, "radius zero for a circle area");
        return "Radius zero for a circle area";
      }
      else if (circle.radiusString() == "")
      {
        alarmMgr.badInput(clientIp, "missing radius for circle area");
        return "Missing radius for circle area";
      }
      else if (circle.invertedString() != "")
      {
        if (!isTrue(circle.invertedString()) && !isFalse(circle.invertedString()))
        {
          std::string details = std::string("bad value for circle/inverted: '") + circle.invertedString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "bad value for circle/inverted: /" + circle.invertedString() + "/";
        }
      }
      else if (circle.center.latitudeString() == "")
      {
        alarmMgr.badInput(clientIp, "missing latitude for circle center");
        return "Missing latitude for circle center";
      }
      else if (circle.center.longitudeString() == "")
      {
        alarmMgr.badInput(clientIp, "missing longitude for circle center");
        return "Missing longitude for circle center";
      }

      double latitude;
      double longitude;
      bool   ok;

      ok = str2double(circle.center.latitudeString().c_str(), &latitude);
      if ((ok == false) || (latitude > 90) || (latitude < -90))
      {
        std::string details = std::string("invalid value for latitude (") + circle.center.latitudeString() + ")";
        alarmMgr.badInput(clientIp, details);
        return "invalid value for latitude";
      }

      ok = str2double(circle.center.longitudeString().c_str(), &longitude);
      if ((ok == false) || (longitude > 180) || (longitude < -180))
      {
        std::string details = std::string("invalid value for longitude: '") + circle.center.longitudeString() + "'";
        alarmMgr.badInput(clientIp, details);
        return "invalid value for longitude";
      }
    }
    else if (areaType == orion::PolygonType)
    {
      if (polygon.vertexList.size() < 3)
      {
        char noOfV[STRING_SIZE_FOR_INT];

        snprintf(noOfV, sizeof(noOfV), "%lu", polygon.vertexList.size());
        std::string details = std::string("too few vertices for a polygon (") + noOfV + " is less than three)";
        alarmMgr.badInput(clientIp, details);

        return "too few vertices for a polygon";
      }
      else if (polygon.invertedString() != "")
      {
        if (!isTrue(polygon.invertedString()) && !isFalse(polygon.invertedString()))
        {
          std::string details = std::string("bad value for polygon/inverted: '") + polygon.invertedString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "bad value for polygon/inverted: /" + polygon.invertedString() + "/";
        }
      }

      for (unsigned int ix = 0; ix < polygon.vertexList.size(); ++ix)
      {
        if (polygon.vertexList[ix]->latitudeString() == "")
        {
          alarmMgr.badInput(clientIp, "missing latitude value for polygon vertex");
          return std::string("missing latitude value for polygon vertex");
        }

        if (polygon.vertexList[ix]->longitudeString() == "")
        {
          alarmMgr.badInput(clientIp, "missing longitude value for polygon vertex");
          return std::string("missing longitude value for polygon vertex");
        }

        double latitude;
        double longitude;
        bool   ok;


        ok = str2double(polygon.vertexList[ix]->latitudeString().c_str(), &latitude);
        if ((ok == false) || (latitude > 90) || (latitude < -90))
        {
          std::string details = std::string("invalid value for latitude: '") + polygon.vertexList[ix]->latitudeString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "invalid value for latitude";
        }

        ok = str2double(polygon.vertexList[ix]->longitudeString().c_str(), &longitude);
        if ((ok == false) || (longitude > 180) || (longitude < -180))
        {
          std::string details = std::string("invalid value for longitude: '") + polygon.vertexList[ix]->longitudeString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "invalid value for longitude";
        }
      }
    }
  }

  if ((type != FIWARE_LOCATION) && (type != FIWARE_LOCATION_DEPRECATED))
  {
    if (type == "")
    {
      alarmMgr.badInput(clientIp, "empty type in restriction scope");
      return "Empty type in restriction scope";
    }

    if (value == "")
    {
      alarmMgr.badInput(clientIp, "empty value in restriction scope");
      return "Empty value in restriction scope";
    }
  }

  if (type == FIWARE_LOCATION_V2)
  {
    if ((areaType == orion::PointType) && (georel.type == "coveredBy"))
    {
      alarmMgr.badInput(clientIp, "Query not supported: point geometry cannot be used with coveredBy georel");
      return "Query not supported: point geometry cannot be used with coveredBy georel";
    }
    else if ((areaType == orion::LineType) && (georel.type == "coveredBy"))
    {
      alarmMgr.badInput(clientIp, "Query not supported: line  geometry cannot be used with coveredBy georel");
      return "Query not supported: line geometry cannot be used with coveredBy georel";
    }
    else if ((areaType == orion::LineType) && (line.pointList.size() < 2))
    {
      alarmMgr.badInput(clientIp, "Query not supported: not enough points for a line");
      return "Query not supported: not enough points for a line";
    }
    else if ((areaType == orion::PolygonType) && (polygon.vertexList.size() < 4))
    {
      alarmMgr.badInput(clientIp, "Query not supported: not enough vertices for a polygon");
      return "Query not supported: not enough vertices for a polygon";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* release -
*/
void Scope::release(void)
{
  // NOTE: georel, circle, box, and point don't use dynamic memory, so they don't need release methods
  polygon.release();
  line.release();

  if (stringFilterP != NULL)
  {
    delete stringFilterP;
    stringFilterP = NULL;
  }

  if (mdStringFilterP != NULL)
  {
    delete mdStringFilterP;
    mdStringFilterP = NULL;
  }
}



/* ****************************************************************************
*
* Scope::areaTypeSet -
*/
void Scope::areaTypeSet(const std::string& areaTypeString)
{
  if      (areaTypeString == "line")    areaType = orion::LineType;
  else if (areaTypeString == "polygon") areaType = orion::PolygonType;
  else if (areaTypeString == "circle")  areaType = orion::CircleType;
  else if (areaTypeString == "point")   areaType = orion::PointType;
  else if (areaTypeString == "box")     areaType = orion::BoxType;
#ifdef ORIONLD
  else if (areaTypeString == "Point")   areaType = orion::PointType;
  else if (areaTypeString == "Polygon") areaType = orion::PolygonType;
#endif
  else                                  areaType = orion::NoArea;
}
