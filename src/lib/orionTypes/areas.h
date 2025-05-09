#ifndef SRC_LIB_ORIONTYPES_AREAS_H_
#define SRC_LIB_ORIONTYPES_AREAS_H_

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
#include <stdlib.h>    // atof
#include <string>
#include <vector>

#include "common/globals.h"

namespace orion
{
/* ****************************************************************************
*
* AreaType -
*/
typedef enum AreaType
{
  NoArea,
  PolygonType,
  PointType,
  LineType,
  BoxType
} AreaType;



/* ****************************************************************************
*
* Point -
*/
class Point
{
 private:
  bool    valid;
  double  lat;
  double  lon;

 public:
  Point();
  Point(double _lat, double _lon);

  void   fill(Point* p);
  double latitude(void) const;
  double longitude(void) const;
  void   latitudeSet(double latitude);
  void   longitudeSet(double longitude);
  bool   equals(Point* p);
};



/* ****************************************************************************
*
* Line -
*/
class Line
{

public:
  ::std::vector<Point*> pointList;

  Line();
  void pointAdd(Point* p);
  void release(void);
};



/* ****************************************************************************
*
* Box -
*/
class Box
{
public:
  Point lowerLeft;
  Point upperRight;

  Box();

  void fill(Point* lowerLeftP, Point* upperRightP);
};



/* ****************************************************************************
*
* Polygon -
*/
class Polygon
{
 public:
  ::std::vector<Point*> vertexList;
  void                  vertexAdd(Point* p);
  void                  release(void);
};



/* ****************************************************************************
*
* Georel -
*/
class Georel
{
public:
  Georel();

  int          parse(const char* in, std::string* errorString);

  std::string  type;
  double       maxDistance;
  double       minDistance;
};



/* ****************************************************************************
*
* Geometry -
*/
class Geometry
{
public:
  Geometry();
  int          parse(const char* in, std::string* errorString);

  std::string  areaType;
};

}

#endif  // SRC_LIB_ORIONTYPES_AREAS_H_
