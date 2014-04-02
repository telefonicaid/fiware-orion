#ifndef ORION_AREAS_H
#define ORION_AREAS_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdlib.h>    // atof
#include <vector>



namespace orion
{
/* ****************************************************************************
*
* AreaType - 
*/
typedef enum AreaType
{
  NoArea,
  CircleType,
  PolygonType
} AreaType;


/* ****************************************************************************
*
* Point - 
*/
class Point
{
private:
  ::std::string _latitude;
  ::std::string _longitude;

public:
  double latitude(void)                        { return atof(_latitude.c_str());  }
  double longitude(void)                       { return atof(_longitude.c_str()); }
  void   latitudeSet(::std::string latitude)   { _latitude  = latitude;           }
  void   longitudeSet(::std::string longitude) { _longitude = longitude;          }
  ::std::string latitudeString(void)           { return _latitude;                }
  ::std::string longitudeString(void)          { return _longitude;               }
};

/* ****************************************************************************
*
* Circle - 
*/
class Circle
{
private:
  ::std::string  _radius; 
  ::std::string  _inverted;

public:
  Point          center;
  bool           inverted(void) { if ((_inverted == "true") || (_inverted == "1")) return true; return false; }
  double         radius(void)   { return atof(_radius.c_str()); }

  ::std::string  radiusString(void)                   { return  _radius;      }
  ::std::string  invertedString(void)                 { return  _inverted;    }
  void           radiusSet(::std::string radius)      { _radius   = radius;   }
  void           invertedSet(::std::string inverted)  { _inverted = inverted; }
};

/* ****************************************************************************
*
* Polygon - 
*/
class Polygon
{
private:
  ::std::string         _inverted;

public:
  ::std::vector<Point*> vertexList;
  bool                  inverted(void)                      { if ((_inverted == "true") || (_inverted == "1")) return true; return false; }
  void                  invertedSet(::std::string inverted) { _inverted = inverted;    }
  ::std::string         invertedString(void)                { return _inverted;        }
  void                  vertexAdd(Point* p)                 { vertexList.push_back(p); }
  void                  release(void)                       { for (unsigned int ix = 0; ix < vertexList.size(); ++ix) delete(vertexList[ix]); vertexList.clear(); }
};

}

#endif
