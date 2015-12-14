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
Point::Point()
{
}



/* ****************************************************************************
*
* Point::Point - 
*/
Point::Point(::std::string latitude, ::std::string longitude): _latitude(latitude), _longitude(longitude)
{

}



/* ****************************************************************************
*
* Point::fill - 
*/
void Point::fill(Point* p)
{
  _latitude  = p->_latitude;
  _longitude = p->_longitude;
}



/* ****************************************************************************
*
* Point::latitude -
*/
double Point::latitude(void) const
{
  return atof(_latitude.c_str());
}


/* ****************************************************************************
*
* Point::longitude -
*/
double Point::longitude(void) const
{
  return atof(_longitude.c_str());
}


/* ****************************************************************************
*
* Point::latitudeSet -
*/
void Point::latitudeSet(::std::string latitude)
{
  _latitude  = latitude;
}



/* ****************************************************************************
*
* Point::longitudeSet -
*/
void Point::longitudeSet(::std::string longitude)
{
  _longitude = longitude;
}



/* ****************************************************************************
*
* Point::latitudeString -
*/
::std::string Point::latitudeString(void)
{
  return _latitude;
}


/* ****************************************************************************
*
* Point::longitudeString -
*/
::std::string Point::longitudeString(void)
{
  return _longitude;
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
  return atof(_radius.c_str());
}



/* ****************************************************************************
*
* Circle::radiusString -
*/
::std::string Circle::radiusString(void) const
{
  return  _radius;
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
void Circle::centerSet(Point* _center)
{
  center.fill(_center);
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
* Geometry::Geometry - 
*/
Geometry::Geometry(): areaType(""), radius(-1), external(false)
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
    if ((items[ix] == "polygon") || (items[ix] == "circle"))
    {
      if (areaType != "")
      {
        *errorString = "polygon/circle present more than once";
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
