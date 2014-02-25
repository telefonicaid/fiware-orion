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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/EntityId.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"
#include "jsonParse/jsonNullTreat.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonUpdateContextAvailabilitySubscriptionRequest.h"



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->ucas.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", reqDataP->ucas.entityIdP));
  reqDataP->ucas.entityIdP->id        = "not in use";
  reqDataP->ucas.entityIdP->type      = "not in use";
  reqDataP->ucas.entityIdP->isPattern = "false";

  reqDataP->ucas.res.entityIdVector.push_back(reqDataP->ucas.entityIdP);
  LM_T(LmtNew, ("After push_back"));

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->ucas.entityIdP->id = value;
   LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->ucas.entityIdP->id.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->ucas.entityIdP->type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->ucas.entityIdP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  if (!isTrue(value) && !isFalse(value))
  {
    LM_E(("invalid isPattern (boolean) value for entity: '%s'", value.c_str()));
    reqDataP->errorString = "invalid isPattern (boolean) value for entity: '" + value + "'";
    return "invalid isPattern (boolean) value for entity: '" + value + "'";
  }

  reqDataP->ucas.entityIdP->isPattern = value;

  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute: '%s'", value.c_str()));

  reqDataP->ucas.res.attributeList.push_back(value);

  return "OK";
}



/* ****************************************************************************
*
* duration - 
*/
static std::string duration(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a duration: '%s'", value.c_str()));

  reqDataP->ucas.res.duration.set(value);

  return "OK";
}



/* ****************************************************************************
*
* restriction - 
*/
static std::string restriction(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a restriction"));

  ++reqDataP->ucas.res.restrictions;

  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  reqDataP->ucas.res.restriction.attributeExpression.set(value);

  return "OK";
}



/* ****************************************************************************
*
* scope - 
*/
static std::string scope(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scope"));

  reqDataP->ucas.scopeP = new Scope();
  reqDataP->ucas.res.restriction.scopeVector.push_back(reqDataP->ucas.scopeP);

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scope type: '%s'", value.c_str()));

  reqDataP->ucas.scopeP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(std::string path, std::string value, ParseData* reqDataP)
{
  if (reqDataP->ucas.scopeP->type == "FIWARE_Location")
  {
    reqDataP->ucas.scopeP->value = "FIWARE_Location";
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqDataP->ucas.scopeP->type.c_str()));
  }
  else
  {
    reqDataP->ucas.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), reqDataP->ucas.scopeP->type.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* circle - 
*/
static std::string circle(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circle"));
  reqDataP->ucas.scopeP->areaType = AreaCircle;
  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static std::string circleCenterLatitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  reqDataP->ucas.scopeP->circle.center.latitude = atof(value.c_str());

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static std::string circleCenterLongitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  reqDataP->ucas.scopeP->circle.center.longitude = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* circleRadius - 
*/
static std::string circleRadius(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  reqDataP->ucas.scopeP->circle.radius = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* circleInverted - 
*/
static std::string circleInverted(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", value.c_str()));

  if (!isTrue(value) && !isFalse(value))
  {
    parseDataP->errorString = "bad string for circle/inverted: '" + value + "'";
    return parseDataP->errorString;
  }
  else
    parseDataP->ucas.scopeP->circle.inverted = isTrue(value);

  return "OK";
}



/* ****************************************************************************
*
* polygon - 
*/
static std::string polygon(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygon"));
  parseDataP->ucas.scopeP->areaType = AreaPolygon;
  return "OK";
}



/* ****************************************************************************
*
* polygonInverted - 
*/
static std::string polygonInverted(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", value.c_str()));

  if (!isTrue(value) && !isFalse(value))
  {
    parseDataP->errorString = "bad string for polygon/inverted: '" + value + "'";
    return parseDataP->errorString;
  }
  else
    parseDataP->ucas.scopeP->polygon.inverted = isTrue(value);

  return "OK";
}



/* ****************************************************************************
*
* polygonVertexList - 
*/
static std::string polygonVertexList(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexList"));
  return "OK";
}



/* ****************************************************************************
*
* polygonVertex - 
*/
static std::string polygonVertex(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertex - creating new vertex for the vertex list"));
  parseDataP->ucas.vertexP = new ScopePoint();
  parseDataP->ucas.scopeP->polygon.vertexList.push_back(parseDataP->ucas.vertexP);
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLatitude - 
*/
static std::string polygonVertexLatitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", value.c_str()));
  parseDataP->ucas.vertexP->latitude = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static std::string polygonVertexLongitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", value.c_str()));
  parseDataP->ucas.vertexP->longitude = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", value.c_str()));
  reqDataP->ucas.res.subscriptionId.set(value);

  return "OK";
}



/* ****************************************************************************
*
* jsonUcasParseVector -
*/
JsonNode jsonUcasParseVector[] =
{
  { "/entities/entity",                    entityId             },
  { "/entities/entity/id",                 entityIdId           },
  { "/entities/entity/type",               entityIdType         },
  { "/entities/entity/isPattern",          entityIdIsPattern    },
  { "/attributes/attribute",               attribute            },
  { "/duration",                           duration             },
  { "/restriction",                        restriction          },
  { "/restriction/attributeExpression",    attributeExpression  },
  { "/restriction/scopes/scope",           scope,               },
  { "/restriction/scopes/scope/type",      scopeType            },
  { "/restriction/scopes/scope/value",     scopeValue           },

  { "/restriction/scopes/scope/value/circle",                              circle                  },
  { "/restriction/scopes/scope/value/circle/center_latitude",              circleCenterLatitude    },
  { "/restriction/scopes/scope/value/circle/center_longitude",             circleCenterLongitude   },
  { "/restriction/scopes/scope/value/circle/radius",                       circleRadius            },
  { "/restriction/scopes/scope/value/circle/inverted",                     circleInverted          },

  { "/restriction/scopes/scope/value/polygon",                             polygon                 },
  { "/restriction/scopes/scope/value/polygon/inverted",                    polygonInverted         },
  { "/restriction/scopes/scope/value/polygon/vertexList",                  polygonVertexList       },
  { "/restriction/scopes/scope/value/polygon/vertexList/vertex",           polygonVertex           },
  { "/restriction/scopes/scope/value/polygon/vertexList/vertex/latitude",  polygonVertexLatitude   },
  { "/restriction/scopes/scope/value/polygon/vertexList/vertex/longitude", polygonVertexLongitude  },

  { "/subscriptionId",                     subscriptionId       },
  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonUcasInit - 
*/
void jsonUcasInit(ParseData* reqDataP)
{
  jsonUcasRelease(reqDataP);

  reqDataP->ucas.entityIdP             = NULL;
  reqDataP->ucas.scopeP                = NULL;
  reqDataP->errorString                = "";

  reqDataP->ucas.res.restrictions      = 0;
  reqDataP->ucas.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* jsonUcasRelease - 
*/
void jsonUcasRelease(ParseData* reqDataP)
{
  reqDataP->ucas.res.release();
}



/* ****************************************************************************
*
* jsonUcasCheck - 
*/
std::string jsonUcasCheck(ParseData* reqData, ConnectionInfo* ciP)
{
   std::string s;
   s = reqData->ucas.res.check(SubscribeContextAvailability, ciP->outFormat, "", reqData->errorString, 0);
   return s;
}



/* ****************************************************************************
*
* jsonUcasPresent - 
*/
void jsonUcasPresent(ParseData* reqDataP)
{
  printf("jsonUcasPresent\n");

  if (!lmTraceIsSet(LmtDump))
    return;

  reqDataP->ucas.res.present("");
}
