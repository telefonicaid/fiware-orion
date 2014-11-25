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
#include <string>
#include <vector>

#include "orionTypes/areas.h"


namespace orion
{
/* ****************************************************************************
*
* Point::latitude -
*/
double Point::latitude(void)
{
  return atof(_latitude.c_str());
}


/* ****************************************************************************
*
* Point::longitude -
*/
double Point::longitude(void)
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
bool Circle::inverted(void)
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
double Circle::radius(void)
{
  return atof(_radius.c_str());
}



/* ****************************************************************************
*
* Circle::radiusString -
*/
::std::string Circle::radiusString(void)
{
  return  _radius;
}



/* ****************************************************************************
*
* Circle::invertedString -
*
*/
::std::string Circle::invertedString(void)
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
* Circle::invertedSet -
*/
void Circle::invertedSet(::std::string inverted)
{
  _inverted = inverted;
}



/* ****************************************************************************
*
* Polygon::inverted - 
*/
bool Polygon::inverted(void)
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
* Polygon::invertedString - 
*/
::std::string Polygon::invertedString(void)
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

}  // namespace orion
