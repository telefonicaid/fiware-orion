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
#include "orionTypes/areas.h"
#include "ngsi/EntityId.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "jsonParse/jsonNullTreat.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonSubscribeContextRequest.h"

using namespace orion;



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  parseDataP->scr.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", parseDataP->scr.entityIdP));
  parseDataP->scr.entityIdP->id        = "";
  parseDataP->scr.entityIdP->type      = "";
  parseDataP->scr.entityIdP->isPattern = "false";

  parseDataP->scr.res.entityIdVector.push_back(parseDataP->scr.entityIdP);
  LM_T(LmtNew, ("After push_back"));

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* parseDataP)
{
   parseDataP->scr.entityIdP->id = value;
   LM_T(LmtParse, ("Set 'id' to '%s' for an entity", parseDataP->scr.entityIdP->id.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* parseDataP)
{
   parseDataP->scr.entityIdP->type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", parseDataP->scr.entityIdP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  parseDataP->scr.entityIdP->isPattern = value;

  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute: '%s'", value.c_str()));

  parseDataP->scr.res.attributeList.push_back(value);

  return "OK";
}



/* ****************************************************************************
*
* reference - 
*/
static std::string reference(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a reference: '%s'", value.c_str()));

  parseDataP->scr.res.reference.set(value);

  return "OK";
}



/* ****************************************************************************
*
* duration - 
*/
static std::string duration(std::string path, std::string value, ParseData* parseDataP)
{
  std::string s;

  LM_T(LmtParse, ("Got a duration: '%s'", value.c_str()));

  parseDataP->scr.res.duration.set(value);

  if ((s = parseDataP->scr.res.duration.check(SubscribeContext, JSON, "", "", 0)) != "OK")
     LM_RE(s, ("error parsing duration '%s': %s", parseDataP->scr.res.duration.get().c_str(), s.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* restriction - 
*/
static std::string restriction(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a restriction"));

  ++parseDataP->scr.res.restrictions;

  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  parseDataP->scr.res.restriction.attributeExpression.set(value);

  return "OK";
}



/* ****************************************************************************
*
* scope - 
*/
static std::string scope(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a scope"));

  parseDataP->scr.scopeP = new Scope();
  parseDataP->scr.res.restriction.scopeVector.push_back(parseDataP->scr.scopeP);

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a scope type: '%s'", value.c_str()));

  parseDataP->scr.scopeP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(std::string path, std::string value, ParseData* parseDataP)
{
  if (parseDataP->scr.scopeP->type == FIWARE_LOCATION)
  {
    //
    // If the scope type is 'FIWARE_Location', then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    parseDataP->scr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", parseDataP->scr.scopeP->type.c_str()));
  }
  else
  {
    parseDataP->scr.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), parseDataP->scr.scopeP->type.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* circle - 
*/
static std::string circle(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circle"));
  parseDataP->scr.scopeP->areaType = orion::CircleType;
  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static std::string circleCenterLatitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  parseDataP->scr.scopeP->circle.center.latitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static std::string circleCenterLongitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  parseDataP->scr.scopeP->circle.center.longitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* circleRadius - 
*/
static std::string circleRadius(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  parseDataP->scr.scopeP->circle.radiusSet(value);
  return "OK";
}



/* ****************************************************************************
*
* circleInverted - 
*/
static std::string circleInverted(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", value.c_str()));

  parseDataP->scr.scopeP->circle.invertedSet(value);

  if (!isTrue(value) && !isFalse(value))
  {
    LM_E(("bad string for circle/inverted: '%s'", value.c_str()));
    parseDataP->errorString = "bad string for circle/inverted: '" + value + "'";
    return parseDataP->errorString;
  }

  return "OK";
}



/* ****************************************************************************
*
* polygon - 
*/
static std::string polygon(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygon"));
  parseDataP->scr.scopeP->areaType = orion::PolygonType;
  return "OK";
}



/* ****************************************************************************
*
* polygonInverted - 
*/
static std::string polygonInverted(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", value.c_str()));

  parseDataP->scr.scopeP->polygon.invertedSet(value);;

  if (!isTrue(value) && !isFalse(value))
  {
    parseDataP->errorString = "bad string for polygon/inverted: '" + value + "'";
    return parseDataP->errorString;
  }

  return "OK";
}



/* ****************************************************************************
*
* polygonVertexList - 
*/
static std::string  polygonVertexList(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexList"));
  return "OK";
}



/* ****************************************************************************
*
* polygonVertex - 
*/
static std::string  polygonVertex(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertex - creating new vertex for the vertex list"));
  parseDataP->scr.vertexP = new orion::Point();
  parseDataP->scr.scopeP->polygon.vertexList.push_back(parseDataP->scr.vertexP);
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLatitude - 
*/
static std::string  polygonVertexLatitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", value.c_str()));
  parseDataP->scr.vertexP->latitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static std::string  polygonVertexLongitude(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", value.c_str()));
  parseDataP->scr.vertexP->longitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* notifyCondition - 
*/
static std::string notifyCondition(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a notifyCondition"));
  parseDataP->scr.notifyConditionP = new NotifyCondition();
  parseDataP->scr.res.notifyConditionVector.push_back(parseDataP->scr.notifyConditionP);
  return "OK";
}



/* ****************************************************************************
*
* notifyConditionRestriction - 
*/
static std::string notifyConditionRestriction(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Notify Condition restriction"));

  parseDataP->scr.notifyConditionP->restriction.set(value);
  return "OK";
}



/* ****************************************************************************
*
* notifyConditionType - 
*/
static std::string notifyConditionType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Notify Condition Type: '%s'", value.c_str()));
  parseDataP->scr.notifyConditionP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* condValue - 
*/
static std::string notifyConditionCondValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Cond Value: '%s'", value.c_str()));
  parseDataP->scr.notifyConditionP->condValueList.push_back(value);
  return "OK";
}



/* ****************************************************************************
*
* throttling - 
*/
static std::string throttling(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a throttling: '%s'", value.c_str()));
  parseDataP->scr.res.throttling.set(value);
  return "OK";
}



/* ****************************************************************************
*
* jsonScrParseVector -
*/
JsonNode jsonScrParseVector[] =
{
  { "/entities",                                               jsonNullTreat              },
  { "/entities/entity",                                        entityId                   },
  { "/entities/entity/id",                                     entityIdId                 },
  { "/entities/entity/type",                                   entityIdType               },
  { "/entities/entity/isPattern",                              entityIdIsPattern          },
  { "/attributes",                                             jsonNullTreat              },
  { "/attributes/attribute",                                   attribute                  },
  { "/reference",                                              reference                  },
  { "/duration",                                               duration                   },
  { "/restriction",                                            restriction                },
  { "/restriction/attributeExpression",                        attributeExpression        },
  { "/restriction/scopes",                                     jsonNullTreat              },
  { "/restriction/scopes/scope",                               scope,                     },
  { "/restriction/scopes/scope/type",                          scopeType                  },
  { "/restriction/scopes/scope/value",                         scopeValue                 },
  { "/restriction/scopes/scope/value/circle",                              circle                  },
  { "/restriction/scopes/scope/value/circle/centerLatitude",               circleCenterLatitude    },
  { "/restriction/scopes/scope/value/circle/centerLongitude",              circleCenterLongitude   },
  { "/restriction/scopes/scope/value/circle/radius",                       circleRadius            },
  { "/restriction/scopes/scope/value/circle/inverted",                     circleInverted          },

  { "/restriction/scopes/scope/value/polygon",                             polygon                 },
  { "/restriction/scopes/scope/value/polygon/vertices",                    polygonVertexList       },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice",            polygonVertex           },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/latitude",   polygonVertexLatitude   },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/longitude",  polygonVertexLongitude  },
  { "/restriction/scopes/scope/value/polygon/inverted",                    polygonInverted         },

  { "/notifyConditions",                                       jsonNullTreat              },
  { "/notifyConditions/notifyCondition",                       notifyCondition            },
  { "/notifyConditions/notifyCondition/type",                  notifyConditionType        },
  { "/notifyConditions/notifyCondition/condValues",            jsonNullTreat              },
  { "/notifyConditions/notifyCondition/condValues/condValue",  notifyConditionCondValue   },
  { "/notifyConditions/notifyCondition/restriction",           notifyConditionRestriction },
  { "/throttling",                                             throttling                 },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonScrInit - 
*/
void jsonScrInit(ParseData* parseDataP)
{
  jsonScrRelease(parseDataP);

  parseDataP->scr.entityIdP              = NULL;
  parseDataP->scr.notifyConditionP       = NULL;
  parseDataP->scr.scopeP                 = NULL;  
  parseDataP->scr.res.restrictions       = 0;
  parseDataP->errorString                = "";
}



/* ****************************************************************************
*
* jsonScrRelease - 
*/
void jsonScrRelease(ParseData* parseDataP)
{
  parseDataP->scr.res.release();
}



/* ****************************************************************************
*
* jsonScrCheck - 
*/
std::string jsonScrCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
   std::string s;
   s = parseDataP->scr.res.check(SubscribeContext, ciP->outFormat, "", parseDataP->errorString, 0);
   return s;
}



/* ****************************************************************************
*
* jsonScrPresent - 
*/
void jsonScrPresent(ParseData* parseDataP)
{
  printf("jsonScrPresent\n");

  if (!lmTraceIsSet(LmtDump))
    return;

  parseDataP->scr.res.present("");
}
