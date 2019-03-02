/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "logMsg/logMsg.h"

#include "common/string.h"
#include "orionTypes/areas.h"


namespace orion
{


/* ****************************************************************************
*
* Point::Point -
*/
Point::Point(): valid(true), lat(0), lon(0)
{
}



/* ****************************************************************************
*
* Point::Point -
*/
Point::Point(::std::string latitude, ::std::string longitude): valid(true)
{
  if ((str2double(latitude.c_str(), &lat) == false) || (str2double(longitude.c_str(), &lon) == false))
  {
    valid = false;
  }
}



/* ****************************************************************************
*
* Point::Point -
*/
Point::Point(double _lat, double _lon): valid(true)
{
  lat = _lat;
  lon = _lon;
}



/* ****************************************************************************
*
* Point::fill -
*/
void Point::fill(Point* p)
{
  lat = p->lat;
  lon = p->lon;
}



/* ****************************************************************************
*
* Point::latitude -
*/
double Point::latitude(void) const
{
  return lat;
}


/* ****************************************************************************
*
* Point::longitude -
*/
double Point::longitude(void) const
{
  return lon;
}


/* ****************************************************************************
*
* Point::latitudeSet -
*/
void Point::latitudeSet(::std::string latitude)
{
  valid = str2double(latitude.c_str(), &lat);
}



/* ****************************************************************************
*
* Point::longitudeSet -
*/
void Point::longitudeSet(::std::string longitude)
{
  valid = str2double(longitude.c_str(), &lon);
}



/* ****************************************************************************
*
* Point::latitudeSet -
*/
void Point::latitudeSet(double latitude)
{
  lat = latitude;
}



/* ****************************************************************************
*
* Point::longitudeSet -
*/
void Point::longitudeSet(double longitude)
{
  lon = longitude;
}



/* ****************************************************************************
*
* Point::equals -
*/
bool Point::equals(Point* p)
{
  if ((p->lat != lat) || (p->lon != lon))
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* Point::latitudeString -
*/
::std::string Point::latitudeString(void)
{
  char cV[STRING_SIZE_FOR_DOUBLE];

  snprintf(cV, sizeof(cV), "%f", lat);
  return cV;
}


/* ****************************************************************************
*
* Point::longitudeString -
*/
::std::string Point::longitudeString(void)
{
  char cV[STRING_SIZE_FOR_DOUBLE];

  snprintf(cV, sizeof(cV), "%f", lon);
  return cV;
}



/* ****************************************************************************
*
* Line::Line -
*/
Line::Line()
{
}



/* ****************************************************************************
*
* Line::pointAdd -
*/
void Line::pointAdd(Point* p)
{
  pointList.push_back(p);
}



/* ****************************************************************************
*
* Line::release -
*/
void Line::release(void)
{
  for (unsigned int ix = 0; ix < pointList.size(); ++ix)
  {
    delete(pointList[ix]);
  }

  pointList.clear();
}



/* ****************************************************************************
*
* Box::Box -
*/
Box::Box()
{
}



/* ****************************************************************************
*
* Box::Box -
*/
Box::Box(Point* lowerLeftP, Point* upperRightP)
{
  lowerLeft.fill(lowerLeftP);
  upperRight.fill(upperRightP);
}



/* ****************************************************************************
*
* Box::fill -
*/
void Box::fill(Point* lowerLeftP, Point* upperRightP)
{
  lowerLeft.fill(lowerLeftP);
  upperRight.fill(upperRightP);
}


/* ****************************************************************************
*
* Circle::inverted -
*/
bool Circle::inverted(void) const
{
  if ((_inverted == "true") || (_inverted == "1"))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* Circle::radius -
*
*/
double Circle::radius(void) const
{
  // NOTE: here we use atof and not str2double on purpose
  float r = atof(_radius.c_str());

  return r;
}



/* ****************************************************************************
*
* Circle::radiusString -
*/
::std::string Circle::radiusString(void) const
{
  return _radius;
}



/* ****************************************************************************
*
* Circle::invertedString -
*
*/
::std::string Circle::invertedString(void) const
{
  return  _inverted;
}



/* ****************************************************************************
*
* Circle::radiusSet -
*/
void Circle::radiusSet(::std::string radius)
{
  _radius   = radius;
}



/* ****************************************************************************
*
* Circle::radiusSet -
*/
void Circle::radiusSet(float radius)
{
  char buffer[64];

  snprintf(buffer, sizeof(buffer), "%f", radius);
  _radius   = buffer;
}



/* ****************************************************************************
*
* Circle::invertedSet -
*/
void Circle::invertedSet(::std::string inverted)
{
  _inverted = inverted;
}



/* ****************************************************************************
*
* Circle::invertedSet -
*/
void Circle::invertedSet(bool inverted)
{
  _inverted = (inverted == true)? "true" : "false";
}



/* ****************************************************************************
*
* Circle::centerSet -
*/
void Circle::centerSet(Point* centerP)
{
  center.fill(centerP);
}



/* ****************************************************************************
*
* Polygon::inverted -
*/
bool Polygon::inverted(void) const
{
  if ((_inverted == "true") || (_inverted == "1"))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* Polygon::invertedSet -
*/
void Polygon::invertedSet(::std::string inverted)
{
  _inverted = inverted;
}



/* ****************************************************************************
*
* Polygon::invertedSet -
*/
void Polygon::invertedSet(bool inverted)
{
  _inverted = (inverted == true)? "true" : "false";
}



/* ****************************************************************************
*
* Polygon::invertedString -
*/
::std::string Polygon::invertedString(void) const
{
  return _inverted;
}



/* ****************************************************************************
*
* Polygon::vertexAdd -
*/
void Polygon::vertexAdd(Point* p)
{
  vertexList.push_back(p);
}



/* ****************************************************************************
*
* Polygon::release -
*/
void Polygon::release(void)
{
  for (unsigned int ix = 0; ix < vertexList.size(); ++ix)
  {
    delete(vertexList[ix]);
  }

  vertexList.clear();
}



/* ****************************************************************************
*
* Georel::Georel -
*/
Georel::Georel(): maxDistance(0), minDistance(0)
{
}



/* ****************************************************************************
*
* Georel::parse -
*/
int Georel::parse(const char* in, std::string* errorString)
{
  std::vector<std::string>  items;
  bool                      maxDistanceSet = false;
  bool                      minDistanceSet = false;

  LM_TMP(("KZ: parsing georel: '%s'", in));

  maxDistance = -1;
  minDistance = -1;

  if (stringSplit(in, ';', items) == 0)
  {
    *errorString = "empty georel";
    return -1;
  }

  for (unsigned int ix = 0; ix < items.size(); ++ix)
  {
    const char* item = items[ix].c_str();

#ifdef ORIONLD
    if ((items[ix] == "near") || (items[ix] == "coveredBy") || (items[ix] == "intersects") || (items[ix] == "equals") || (items[ix] == "disjoint") || (items[ix] == "within"))
#else
    if ((items[ix] == "near") || (items[ix] == "coveredBy") || (items[ix] == "intersects") || (items[ix] == "equals") || (items[ix] == "disjoint"))
#endif
    {
      if (type != "")
      {
        *errorString = "georel type present more than once";
        return -1;
      }

      type = items[ix];
    }
    else if (strncmp(item, "maxDistance:", 12) == 0)
    {
      if (maxDistanceSet)
      {
        *errorString = "maxDistance present more than once";
        return -1;
      }

      if (str2double(&item[12], &maxDistance) == false)
      {
        *errorString = "invalid number for maxDistance";
        return -1;
      }

      maxDistanceSet = true;
    }
    else if (strncmp(item, "minDistance:", 12) == 0)
    {
      if (minDistanceSet)
      {
        *errorString = "minDistance present more than once";
        return -1;
      }

      if (str2double(&item[12], &minDistance) == false)
      {
        *errorString = "invalid number for minDistance";
        return -1;
      }

      minDistanceSet = true;
    }
#ifdef ORIONLD
    else if (strncmp(item, "maxDistance==", 13) == 0)
    {
      LM_TMP(("KZ: Got maxDistance==X"));
      if (maxDistanceSet)
      {
        *errorString = "maxDistance present more than once";
        return -1;
      }

      if (str2double(&item[13], &maxDistance) == false)
      {
        *errorString = "invalid number for maxDistance";
        return -1;
      }

      maxDistanceSet = true;
    }
    else if (strncmp(item, "minDistance==", 13) == 0)
    {
      LM_TMP(("KZ: Got minDistance==X"));
      if (minDistanceSet)
      {
        *errorString = "minDistance present more than once";
        return -1;
      }

      if (str2double(&item[13], &minDistance) == false)
      {
        *errorString = "invalid number for minDistance";
        return -1;
      }

      minDistanceSet = true;
    }
#endif
    else
    {
      *errorString = "Invalid modifier in georel parameter";
      LM_E(("Invalid modifier in georel parameter: '%s'", item));
      return -1;
    }
  }

  if (maxDistanceSet && (type != "near"))
  {
    *errorString = std::string("maxDistance erroneously used with georel /") + type + "/";
    return -1;
  }

  if (minDistanceSet && (type != "near"))
  {
    *errorString = std::string("minDistance erroneously used with georel /") + type + "/";
    return -1;
  }

  if ((type == "near") && !maxDistanceSet && !minDistanceSet)
  {
    *errorString = "georel /near/ without either minDistance nor maxDistance";
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* Georel::fill -
*/
void Georel::fill(Georel* georelP)
{
  type        = georelP->type;
  maxDistance = georelP->maxDistance;
  minDistance = georelP->minDistance;
}



/* ****************************************************************************
*
* Geometry::Geometry -
*/
Geometry::Geometry(): areaType(""), radius(-1), external(false)
{
}



/* ****************************************************************************
*
* Geometry::parse -
*/
int Geometry::parse(ApiVersion apiVersion, const char* in, std::string* errorString)
{
  std::vector<std::string> items;

  if (stringSplit(in, ';', items) == 0)
  {
    *errorString = "empty geometry";
    return -1;
  }

  for (unsigned int ix = 0; ix < items.size(); ++ix)
  {
#ifdef ORIONLD
    if (apiVersion == NGSI_LD_V1)
    {
      if (items[ix] == "Point")
        items[ix] = "point";
      else if (items[ix] == "Polygon")
        items[ix] = "polygon";
    }
#endif    
    if ((apiVersion == V1) && ((items[ix] == "polygon") || (items[ix] == "circle")))
    {
      if (areaType != "")
      {
        *errorString = "polygon/circle present more than once";
        return -1;
      }
      areaType = items[ix];
    }
    else if ((apiVersion != V1) && ((items[ix] == "point") || (items[ix] == "line") || (items[ix] == "box") || (items[ix] == "polygon")))
    {
      if (areaType != "")
      {
        *errorString = "geometry-type present more than once";
        return -1;
      }

      areaType = items[ix];
    }
    else if (strncmp(items[ix].c_str(), "radius", 6) == 0)
    {
      radius = atoi((char*) &items[ix].c_str()[7]);

      if (radius == 0)
      {
        *errorString = "Invalid value of /radius/";
        return -1;
      }
    }
    else if (items[ix] == "external")
    {
      external = true;
    }
    else
    {
      LM_E(("items[ix] == '%s' - invalid selector in geometry specification", items[ix].c_str()));
      *errorString = "Invalid selector in geometry specification";
      return -1;
    }
  }

  if ((areaType == "circle") && (radius == -1))
  {
    *errorString = "no radius for circle";
    return -1;
  }

  if ((areaType == "polygon") && (radius != -1))
  {
    *errorString = "radius set for polygon";
    return -1;
  }

  return 0;
}

}  // namespace orion
