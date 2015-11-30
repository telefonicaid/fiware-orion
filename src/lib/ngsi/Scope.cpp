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
#include "common/Format.h"
#include "common/string.h"
#include "ngsi/Scope.h"
#include "parse/forbiddenChars.h"

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
}



/* ****************************************************************************
*
* Scope::fill - 
*/
int Scope::fill
(
  orion::Geometry*                 geometry,
  const std::vector<std::string>&  coordsV,
  std::string*                     errorString
)
{
  if (geometry->areaType == "circle")
  {
    areaType = orion::CircleType;

    circle.radiusSet(geometry->radius);
    circle.invertedSet(geometry->external);

    std::vector<std::string>  coords;

    if (stringSplit(coordsV[0], ',', coords) == 2)
    {
      std::string latitude  = coords[0];
      std::string longitude = coords[1];

      Point p(latitude, longitude);
      circle.centerSet(&p);
    }
    else
    {
      *errorString = "invalid coordinate-set for circle";
      return -1;
    }
  }
  else if (geometry->areaType == "polygon")
  {
    areaType = orion::PolygonType;
    
    polygon.invertedSet(geometry->external);

    for (unsigned int ix = 0; ix < coordsV.size(); ++ix)
    {
      std::vector<std::string>  coords;

      if (stringSplit(coordsV[ix], ',', coords) == 2)
      {
        std::string latitude  = coords[0];
        std::string longitude = coords[1];

        // FIXME P1:  we should check 'valid number' of coords[0] and coords[1]
        Point* p = new Point(latitude, longitude);
        polygon.vertexAdd(p);
      }
      else
      {
        *errorString = "invalid coordinate-set for polygon";
        return -1;
      }
    }
  }
  else
  {
    areaType = orion::NoArea;
    
    *errorString = "invalid area-type";
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* Scope::render -
*/
std::string Scope::render(Format format, const std::string& indent, bool notLastInVector)
{
  std::string out      = "";
  std::string tag      = "operationScope";
  const char* tTag     = (format == XML)? "scopeType"  : "type";
  const char* vTag     = (format == XML)? "scopeValue" : "value";

  out += startTag(indent, tag, tag, format, false, false);
  out += valueTag(indent + "  ", tTag, type, format, true);
  out += valueTag(indent + "  ", vTag, value, format);
  out += endTag(indent, tag, format, notLastInVector);

  return out;
}



/* ****************************************************************************
*
* Scope::check -
*/
std::string Scope::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  //
  // Check for forbidden characters
  //
  if (forbiddenChars(type.c_str()))
  {
    return "illegal chars in scope type";
  }

  if (type != SCOPE_TYPE_SIMPLE_QUERY)
  {
    if (forbiddenChars(value.c_str()))
    {
      return "illegal chars in scope";
    }
  }

  if (type == FIWARE_LOCATION || type == FIWARE_LOCATION_DEPRECATED)
  {
    if (areaType == orion::CircleType)
    {
      if (circle.radiusString() == "0")
      {
        LM_W(("Bad Input (radius zero for a circle area)"));
        return "Radius zero for a circle area";
      }
      else if (circle.radiusString() == "")
      {
        LM_W(("Bad Input (missing radius for circle area)"));
        return "Missing radius for circle area";
      }
      else if (circle.invertedString() != "")
      {
        if (!isTrue(circle.invertedString()) && !isFalse(circle.invertedString()))
        {
          LM_W(("Bad Input (bad value for circle/inverted: '%s')", circle.invertedString().c_str()));
          return "bad value for circle/inverted: /" + circle.invertedString() + "/";
        }
      }
      else if (circle.center.latitudeString() == "")
      {
        LM_W(("Bad Input (missing latitude for circle center)"));
        return "Missing latitude for circle center";
      }
      else if (circle.center.longitudeString() == "")
      {
        LM_W(("Bad Input (missing longitude for circle center)"));
        return "Missing longitude for circle center";
      }

      float latitude = atof(circle.center.latitudeString().c_str());
      if ((latitude > 90) || (latitude < -90))
      {
        LM_W(("Bad Input (invalid value for latitude (%s))", circle.center.latitudeString().c_str()));
        return "invalid value for latitude";
      }

      float longitude = atof(circle.center.longitudeString().c_str());
      if ((longitude > 180) || (longitude < -180))
      {
        LM_W(("Bad Input (invalid value for longitude: '%s')", circle.center.longitudeString().c_str()));
        return "invalid value for longitude";
      }
    }
    else if (areaType == orion::PolygonType)
    {
      if (polygon.vertexList.size() < 3)
      {
        LM_W(("Bad Input (too few vertices for a polygon (%d is less than three))", polygon.vertexList.size()));
        return "too few vertices for a polygon";
      }
      else if (polygon.invertedString() != "")
      {
        if (!isTrue(polygon.invertedString()) && !isFalse(polygon.invertedString()))
        {
          LM_W(("Bad Input (bad value for polygon/inverted: '%s')", polygon.invertedString().c_str()));
          return "bad value for polygon/inverted: /" + polygon.invertedString() + "/";
        }
      }

      for (unsigned int ix = 0; ix < polygon.vertexList.size(); ++ix)
      {
        if (polygon.vertexList[ix]->latitudeString() == "")
        {
          LM_W(("Bad Input (missing latitude value for polygon vertex)"));
          return std::string("missing latitude value for polygon vertex");
        }

        if (polygon.vertexList[ix]->longitudeString() == "")
        {
          LM_W(("Bad Input (missing longitude value for polygon vertex)"));
          return std::string("missing longitude value for polygon vertex");
        }

        float latitude = atof(polygon.vertexList[ix]->latitudeString().c_str());
        if ((latitude > 90) || (latitude < -90))
        {
          LM_W(("Bad Input (invalid value for latitude: '%s')", polygon.vertexList[ix]->latitudeString().c_str()));
          return "invalid value for latitude";
        }

        float longitude = atof(polygon.vertexList[ix]->longitudeString().c_str());
        if ((longitude > 180) || (longitude < -180))
        {
          LM_W(("Bad Input (invalid value for longitude: '%s')", polygon.vertexList[ix]->longitudeString().c_str()));
          return "invalid value for longitude";
        }
      }
    }
  }

  if ((type != FIWARE_LOCATION) && (type != FIWARE_LOCATION_DEPRECATED))
  {
    if (type == "")
    {
      LM_W(("Bad Input (empty type in restriction scope)"));
      return "Empty type in restriction scope";
    }

    if (value == "")
    {
      LM_W(("Bad Input (empty value in restriction scope)"));
      return "Empty value in restriction scope";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* Scope::present -
*/
void Scope::present(const std::string& indent, int ix)
{
  if (ix == -1)
  {
    LM_T(LmtPresent, ("%sScope:",       indent.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sScope %d:",    
		      indent.c_str(),
		      ix));
  }

  LM_T(LmtPresent, ("%s  Type:     '%s'", 
		    indent.c_str(), 
		    type.c_str()));
  
  if (oper != "")
    LM_T(LmtPresent, ("%s  Operator: '%s'", 
		      indent.c_str(), 
		      oper.c_str()));

  if (areaType == orion::NoArea)
  {
    LM_T(LmtPresent, ("%s  Value:    %s", 
		      indent.c_str(), 
		      value.c_str()));
  }
  else if (areaType == orion::CircleType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Circle Area:", indent.c_str()));
    LM_T(LmtPresent, ("%s    Radius:     %s", 
	  indent.c_str(), 
	  circle.radiusString().c_str()));
    LM_T(LmtPresent, ("%s    Longitude:  %s", 
	  indent.c_str(), 
	  circle.center.longitudeString().c_str()));
    LM_T(LmtPresent, ("%s    Latitude:   %s", 
	  indent.c_str(), 
	  circle.center.latitudeString().c_str()));
    LM_T(LmtPresent, ("%s    Inverted:   %s", 
	  indent.c_str(), 
	  circle.invertedString().c_str()));
  }
  else if (areaType == orion::PolygonType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Polygon Area (%lu vertices):", 
		      indent.c_str(), 
		      polygon.vertexList.size()));

    LM_T(LmtPresent, ("%s    Inverted:   %s", 
		      indent.c_str(), 
		      polygon.invertedString().c_str()));
    for (unsigned int ix = 0; ix < polygon.vertexList.size(); ++ix)
    {
      LM_T(LmtPresent, ("%s    Vertex %d", 
			indent.c_str(), 
			ix));
      LM_T(LmtPresent, ("%s      Longitude:  %s", 
			indent.c_str(), 
			polygon.vertexList[ix]->longitudeString().c_str()));
      LM_T(LmtPresent, ("%s      Latitude:   %s", 
			indent.c_str(), 
			polygon.vertexList[ix]->latitudeString().c_str()));
    }
  }
}



/* ****************************************************************************
*
* release -
*/
void Scope::release(void)
{
  polygon.release();
}
