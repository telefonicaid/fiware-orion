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
* Box::fill -
*/
void Box::fill(Point* lowerLeftP, Point* upperRightP)
{
  lowerLeft.fill(lowerLeftP);
  upperRight.fill(upperRightP);
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

    if ((items[ix] == "near") || (items[ix] == "coveredBy") || (items[ix] == "intersects") || (items[ix] == "equals") || (items[ix] == "disjoint"))
    {
      if (!type.empty())
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
    else
    {
      *errorString = "Invalid modifier in georel parameter";
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
* Geometry::Geometry -
*/
Geometry::Geometry(): areaType("")
{
}



/* ****************************************************************************
*
* Geometry::parse -
*/
int Geometry::parse(const char* in, std::string* errorString)
{
  std::vector<std::string> items;

  if (stringSplit(in, ';', items) == 0)
  {
    *errorString = "empty geometry";
    return -1;
  }

  for (unsigned int ix = 0; ix < items.size(); ++ix)
  {
    if (((items[ix] == "point") || (items[ix] == "line") || (items[ix] == "box") || (items[ix] == "polygon")))
    {
      if (!areaType.empty())
      {
        *errorString = "geometry-type present more than once";
        return -1;
      }

      areaType = items[ix];
    }
    else
    {
      *errorString = "Invalid selector in geometry specification";
      return -1;
    }
  }

  return 0;
}

}  // namespace orion
