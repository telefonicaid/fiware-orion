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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/string.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/areas.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonQueryContextRequest.h"
#include "parse/nullTreat.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"

using namespace orion;



/* ****************************************************************************
*
* entityId -
*/
static std::string entityId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->qcr.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", reqDataP->qcr.entityIdP));
  reqDataP->qcr.entityIdP->id        = "";
  reqDataP->qcr.entityIdP->type      = "";
  reqDataP->qcr.entityIdP->isPattern = "false";

  reqDataP->qcr.res.entityIdVector.push_back(reqDataP->qcr.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId -
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP->id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->qcr.entityIdP->id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType -
*/
static std::string entityIdType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->qcr.entityIdP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern -
*/
static std::string entityIdIsPattern(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  reqDataP->qcr.entityIdP->isPattern = value;
  if (!isTrue(value) && !isFalse(value))
  {
    return "invalid isPattern value for entity: /" + value + "/";
  }

  return "OK";
}



/* ****************************************************************************
*
* attribute -
*/
static std::string attribute(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute: '%s'", value.c_str()));

  if (value == "")
  {
    alarmMgr.badInput(clientIp, "empty attribute name");
    reqDataP->errorString = "Empty attribute name";
  }

  reqDataP->qcr.res.attributeList.push_back(value);

  return "OK";
}



/* ****************************************************************************
*
* attributeList -
*/
static std::string attributeList(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeList: '%s'", value.c_str()));
  return "OK";
}



/* ****************************************************************************
*
* restriction -
*/
static std::string restriction(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.res.restrictions += 1;
  return "OK";
}



/* ****************************************************************************
*
* attributeExpression -
*/
static std::string attributeExpression(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  reqDataP->qcr.res.restriction.attributeExpression.set(value);

  if (value == "")
  {
    alarmMgr.badInput(clientIp, "empty attribute expression");
    reqDataP->errorString = "Empty attribute expression";
  }

  return "OK";
}



/* ****************************************************************************
*
* operationScope -
*/
static std::string operationScope(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an operationScope"));

  reqDataP->qcr.scopeP = new Scope();
  reqDataP->qcr.res.restriction.scopeVector.push_back(reqDataP->qcr.scopeP);
  reqDataP->qcr.scopeP->type  = "";
  reqDataP->qcr.scopeP->value = "";

  return "OK";
}



/* ****************************************************************************
*
* scopeType -
*/
static std::string scopeType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.scopeP->type = value;
  LM_T(LmtParse, ("Set scope 'type' to '%s' for a scope", reqDataP->qcr.scopeP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* scopeValue -
*/
static std::string scopeValue(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  if (reqDataP->qcr.scopeP->type == FIWARE_LOCATION || reqDataP->qcr.scopeP->type == FIWARE_LOCATION_DEPRECATED)
  {
    //
    // If the scope type is FIWARE_LOCATION' (or its deprecated variant), then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    reqDataP->qcr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqDataP->qcr.scopeP->type.c_str()));
  }
  else if (reqDataP->qcr.scopeP->type == FIWARE_LOCATION_V2)
  {
    // Mark the value of the scope to be of complex nature, v2
    reqDataP->qcr.scopeP->value = FIWARE_LOCATION_V2;
  }
  else if (reqDataP->qcr.scopeP->type == SCOPE_TYPE_SIMPLE_QUERY)
  {
    std::string errorString;

    reqDataP->qcr.scopeP->value = value;

    reqDataP->qcr.scopeP->stringFilterP = new StringFilter(SftQ);
    bool b = reqDataP->qcr.scopeP->stringFilterP->parse(value.c_str(), &errorString);

    if (b != true)
    {
      delete reqDataP->qcr.scopeP->stringFilterP;
      reqDataP->qcr.scopeP->stringFilterP = NULL;

      return errorString;
    }
  }
  else if (reqDataP->qcr.scopeP->type == SCOPE_TYPE_SIMPLE_QUERY_MD)
  {
    std::string errorString;

    reqDataP->qcr.scopeP->value = value;

    reqDataP->qcr.scopeP->mdStringFilterP = new StringFilter(SftMq);
    bool b = reqDataP->qcr.scopeP->mdStringFilterP->parse(value.c_str(), &errorString);

    if (b != true)
    {
      delete reqDataP->qcr.scopeP->mdStringFilterP;
      reqDataP->qcr.scopeP->mdStringFilterP = NULL;

      return errorString;
    }
  }
  else
  {
    reqDataP->qcr.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), reqDataP->qcr.scopeP->type.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* circle -
*/
static std::string circle(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circle"));
  reqDataP->qcr.scopeP->areaType = orion::CircleType;

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude -
*/
static std::string circleCenterLatitude(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.center.latitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude -
*/
static std::string circleCenterLongitude(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.center.longitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* circleRadius -
*/
static std::string circleRadius(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.radiusSet(value);

  return "OK";
}



/* ****************************************************************************
*
* circleInverted -
*/
static std::string circleInverted(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", value.c_str()));

  parseDataP->qcr.scopeP->circle.invertedSet(value);
  if (!isTrue(value) && !isFalse(value))
  {
    parseDataP->errorString = "bad string for circle/inverted: /" + value + "/";
    return parseDataP->errorString;
  }

  return "OK";
}



/* ****************************************************************************
*
* polygon -
*/
static std::string polygon(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygon"));
  parseDataP->qcr.scopeP->areaType = orion::PolygonType;

  return "OK";
}



/* ****************************************************************************
*
* polygonInverted -
*/
static std::string polygonInverted(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", value.c_str()));

  parseDataP->qcr.scopeP->polygon.invertedSet(value);
  if (!isTrue(value) && !isFalse(value))
  {
    parseDataP->errorString = "bad string for polygon/inverted: /" + value + "/";
    return parseDataP->errorString;
  }

  return "OK";
}



/* ****************************************************************************
*
* polygonVertexList -
*/
static std::string polygonVertexList(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexList"));

  return "OK";
}



/* ****************************************************************************
*
* polygonVertex -
*/
static std::string polygonVertex(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertex - creating new vertex for the vertex list"));
  parseDataP->qcr.vertexP = new orion::Point();
  parseDataP->qcr.scopeP->polygon.vertexAdd(parseDataP->qcr.vertexP);
  // parseDataP->qcr.scopeP->polygon.vertexList.push_back(parseDataP->qcr.vertexP);

  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLatitude -
*/
static std::string polygonVertexLatitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", value.c_str()));
  parseDataP->qcr.vertexP->latitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLongitude -
*/
static std::string polygonVertexLongitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", value.c_str()));
  parseDataP->qcr.vertexP->longitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* georel -
*/
static std::string georel(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a georel"));
  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "georel in scope value used with non-api-v2 scope";
  }

  parseDataP->qcr.coords  = 0;
  parseDataP->qcr.pointNo = 0;
  
  return "OK";
}



/* ****************************************************************************
*
* georelValue -
*/
static std::string georelValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a georel value"));
  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "georel in scope value used with non-api-v2 scope";
  }

  if (strncmp(value.c_str(), "maxDistance:", strlen("maxDistance:")) == 0)
  {
    const char* maxDistanceP = &value.c_str()[strlen("maxDistance:")];
    if (str2double(maxDistanceP, &parseDataP->qcr.scopeP->georel.maxDistance) != true)
    {
      return "georel with a maxDistance that is not a number";
    }
  }
  else if (strncmp(value.c_str(), "minDistance:", strlen("minDistance:")) == 0)
  {
    const char* minDistanceP = &value.c_str()[strlen("minDistance:")];
    if (str2double(minDistanceP, &parseDataP->qcr.scopeP->georel.minDistance) != true)
    {
      return "georel with a minDistance that is not a number";
    }
  }
  else
  {
    // Check for valid georel.type comes later
    parseDataP->qcr.scopeP->georel.type = value;
  }

  return "OK";
}



/* ****************************************************************************
*
* geometry -
*/
static std::string geometry(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a geometry"));

  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "geometry in scope value used with non-api-v2 scope";
  }

  parseDataP->qcr.coords  = 0;
  parseDataP->qcr.pointNo = 0;
  parseDataP->qcr.scopeP->areaTypeSet(value);

  return "OK";
}


/* ****************************************************************************
*
* coords -
*/
static std::string coords(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a coords"));

  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "coords in scope value used with non-api-v2 scope";
  }

  return "OK";
}



/* ****************************************************************************
*
* coord -
*/
static std::string coord(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a coord-pair"));

  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "coords in scope value used with non-api-v2 scope";
  }

  parseDataP->qcr.coords    = 0;
  parseDataP->qcr.pointNo  += 1;

  return "OK";
}



/* ****************************************************************************
*
* coordValue -
*/
static std::string coordValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a coordinate"));

  if (parseDataP->qcr.scopeP->type != FIWARE_LOCATION_V2)
  {
    return "coords in scope value used with non-api-v2 scope";
  }


  //
  // Decide where to put the point ...
  //
  double coordVal;

  if (str2double(value.c_str(), &coordVal) == false)
  {
    return "invalid coordinate";
  }

  switch (parseDataP->qcr.scopeP->areaType)
  {
  case orion::PointType:
    if (parseDataP->qcr.coords == 0)
    {
      parseDataP->qcr.scopeP->point.latitudeSet(coordVal);
    }
    else if (parseDataP->qcr.coords == 1)
    {
      parseDataP->qcr.scopeP->point.longitudeSet(coordVal);
    }
    else
    {
      return "too many coordinates for a Point geometry";
    }
    break;

  case orion::PolygonType:
    if (parseDataP->qcr.coords == 0)
    {
      parseDataP->qcr.vertexP = new Point(coordVal, 0);
      parseDataP->qcr.scopeP->polygon.vertexAdd(parseDataP->qcr.vertexP);
    }
    else if (parseDataP->qcr.coords == 1)
    {
      parseDataP->qcr.vertexP->longitudeSet(coordVal);
    }
    else
    {
      return "too many coordinates for a Point in Polygon geometry";
    }
    break;

  case orion::LineType:
    if (parseDataP->qcr.coords == 0)
    {
      parseDataP->qcr.vertexP = new Point(coordVal, 0);
      parseDataP->qcr.scopeP->line.pointAdd(parseDataP->qcr.vertexP);
    }
    else if (parseDataP->qcr.coords == 1)
    {
      parseDataP->qcr.vertexP->longitudeSet(coordVal);
    }
    else
    {
      return "too many coordinates for a Point in Line geometry";
    }
    break;

  case orion::BoxType:
    if (parseDataP->qcr.pointNo > 2)
    {
      return "too many points for a Box geoetry";
    }

    if (parseDataP->qcr.coords == 0)
    {
      if (parseDataP->qcr.pointNo == 1)
      {
        parseDataP->qcr.scopeP->box.lowerLeft.latitudeSet(coordVal);
      }
      else
      {
        parseDataP->qcr.scopeP->box.upperRight.latitudeSet(coordVal);
      }
    }
    else if (parseDataP->qcr.coords == 1)
    {
      if (parseDataP->qcr.pointNo == 1)
      {
        parseDataP->qcr.scopeP->box.lowerLeft.longitudeSet(coordVal);
      }
      else
      {
        parseDataP->qcr.scopeP->box.upperRight.longitudeSet(coordVal);
      }
    }
    else
    {
      return "too many coordinates for a Point in Box geometry";
    }
    break;

  case orion::NoArea:
  case orion::CircleType:
    return "received coordinates for invalid geometry";
    break;
  }

  parseDataP->qcr.coords += 1;
  return "OK";
}



/* ****************************************************************************
*
* qcrParseVector -
*/
JsonNode jsonQcrParseVector[] =
{
  { "/entities",                                                           jsonNullTreat           },
  { "/entities/entity",                                                    entityId                },
  { "/entities/entity/id",                                                 entityIdId              },
  { "/entities/entity/type",                                               entityIdType            },
  { "/entities/entity/isPattern",                                          entityIdIsPattern       },

  { "/attributes",                                                         attributeList           },
  { "/attributes/attribute",                                               attribute               },

  { "/restriction",                                                        restriction             },
  { "/restriction/attributeExpression",                                    attributeExpression     },
  { "/restriction/scopes",                                                 jsonNullTreat           },
  { "/restriction/scopes/scope",                                           operationScope          },
  { "/restriction/scopes/scope/type",                                      scopeType               },
  { "/restriction/scopes/scope/value",                                     scopeValue              },

  { "/restriction/scopes/scope/value/circle",                              circle                  },
  { "/restriction/scopes/scope/value/circle/centerLatitude",               circleCenterLatitude    },
  { "/restriction/scopes/scope/value/circle/centerLongitude",              circleCenterLongitude   },
  { "/restriction/scopes/scope/value/circle/radius",                       circleRadius            },
  { "/restriction/scopes/scope/value/circle/inverted",                     circleInverted          },

  { "/restriction/scopes/scope/value/polygon",                             polygon                 },
  { "/restriction/scopes/scope/value/polygon/inverted",                    polygonInverted         },
  { "/restriction/scopes/scope/value/polygon/vertices",                    polygonVertexList       },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice",            polygonVertex           },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/latitude",   polygonVertexLatitude   },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/longitude",  polygonVertexLongitude  },

  // APIv2 geolocation in v1
  { "/restriction/scopes/scope/value/geometry",                            geometry                },
  { "/restriction/scopes/scope/value/georel",                              georel                  },
  { "/restriction/scopes/scope/value/georel/geore",                        georelValue             },
  { "/restriction/scopes/scope/value/coords",                              coords                  },
  { "/restriction/scopes/scope/value/coords/coord",                        coord                   },
  { "/restriction/scopes/scope/value/coords/coord/coor",                   coordValue              },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonQcrInit -
*/
void jsonQcrInit(ParseData* reqDataP)
{
  jsonQcrRelease(reqDataP);

  reqDataP->qcr.entityIdP              = NULL;
  reqDataP->qcr.scopeP                 = NULL;
  reqDataP->errorString                = "";

  reqDataP->qcr.res.restrictions           = 0;
  reqDataP->qcr.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* jsonQcrRelease -
*/
void jsonQcrRelease(ParseData* reqDataP)
{
  reqDataP->qcr.res.release();
}



/* ****************************************************************************
*
* jsonQcrCheck -
*/
std::string jsonQcrCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
  return reqDataP->qcr.res.check(ciP->apiVersion, asJsonObject, reqDataP->errorString);
}
