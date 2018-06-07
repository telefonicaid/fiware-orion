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
#include "alarmMgr/alarmMgr.h"
#include "orionTypes/areas.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "parse/nullTreat.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonUpdateContextSubscriptionRequest.h"

using namespace orion;



/* ****************************************************************************
*
* duration - 
*/
static std::string duration(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  std::string s;

  LM_T(LmtParse, ("Got a duration: '%s'", value.c_str()));

  parseDataP->ucsr.res.duration.set(value);

  // The failure is postponed until the 'check' step to not miss the subscriptionId
  if ((s = parseDataP->ucsr.res.duration.check()) != "OK")
  {
    std::string details = std::string("error parsing duration '") + parseDataP->ucsr.res.duration.get() + "': " + s;
    alarmMgr.badInput(clientIp, details);
  }

  return "OK";
}



/* ****************************************************************************
*
* restriction - 
*/
static std::string restriction(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a restriction"));

  ++parseDataP->ucsr.res.restrictions;

  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  parseDataP->ucsr.res.restriction.attributeExpression.set(value);

  return "OK";
}



/* ****************************************************************************
*
* scope - 
*/
static std::string scope(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a scope"));

  parseDataP->ucsr.scopeP = new Scope();
  parseDataP->ucsr.res.restriction.scopeVector.push_back(parseDataP->ucsr.scopeP);

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a scope type: '%s'", value.c_str()));

  parseDataP->ucsr.scopeP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  if (parseDataP->ucsr.scopeP->type == FIWARE_LOCATION || parseDataP->ucsr.scopeP->type == FIWARE_LOCATION_DEPRECATED)
  {
    //
    // If the scope type is FIWARE_LOCATION (or its deprecated variant), then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    parseDataP->ucsr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", parseDataP->ucsr.scopeP->type.c_str()));
  }
  else
  {
    parseDataP->ucsr.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), parseDataP->ucsr.scopeP->type.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* circle - 
*/
static std::string circle(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circle"));
  parseDataP->ucsr.scopeP->areaType = orion::CircleType;
  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static std::string circleCenterLatitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  parseDataP->ucsr.scopeP->circle.center.latitudeSet(value);

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static std::string circleCenterLongitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  parseDataP->ucsr.scopeP->circle.center.longitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* circleRadius - 
*/
static std::string circleRadius(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  parseDataP->ucsr.scopeP->circle.radiusSet(value);
  return "OK";
}



/* ****************************************************************************
*
* circleInverted - 
*/
static std::string circleInverted(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", value.c_str()));

  parseDataP->ucsr.scopeP->circle.invertedSet(value);
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
  parseDataP->ucsr.scopeP->areaType = orion::PolygonType;
  return "OK";
}



/* ****************************************************************************
*
* polygonInverted - 
*/
static std::string polygonInverted(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", value.c_str()));

  parseDataP->ucsr.scopeP->polygon.invertedSet(value);
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
  parseDataP->ucsr.vertexP = new orion::Point();
  parseDataP->ucsr.scopeP->polygon.vertexAdd(parseDataP->ucsr.vertexP);
  // parseDataP->ucsr.scopeP->polygon.vertexList.push_back(parseDataP->ucsr.vertexP);
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLatitude - 
*/
static std::string polygonVertexLatitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", value.c_str()));
  parseDataP->ucsr.vertexP->latitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static std::string polygonVertexLongitude(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", value.c_str()));
  parseDataP->ucsr.vertexP->longitudeSet(value);
  return "OK";
}



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", value.c_str()));

  parseDataP->ucsr.res.subscriptionId.set(value);

  return "OK";
}



/* ****************************************************************************
*
* notifyCondition - 
*/
static std::string notifyCondition(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a notifyCondition"));
  parseDataP->ucsr.notifyConditionP = new NotifyCondition();
  parseDataP->ucsr.res.notifyConditionVector.push_back(parseDataP->ucsr.notifyConditionP);
  return "OK";
}



/* ****************************************************************************
*
* notifyConditionRestriction - 
*/
static std::string notifyConditionRestriction(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Notify Condition restriction"));

  parseDataP->ucsr.notifyConditionP->restriction.set(value);
  return "OK";
}



/* ****************************************************************************
*
* notifyConditionType - 
*/
static std::string notifyConditionType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Notify Condition Type: '%s'", value.c_str()));
  parseDataP->ucsr.notifyConditionP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* condValue - 
*/
static std::string notifyConditionCondValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a Cond Value: '%s'", value.c_str()));
  parseDataP->ucsr.notifyConditionP->condValueList.push_back(value);
  return "OK";
}



/* ****************************************************************************
*
* throttling - 
*/
static std::string throttling(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a throttling: '%s'", value.c_str()));
  parseDataP->ucsr.res.throttling.set(value);
  return "OK";
}



/* ****************************************************************************
*
* jsonUcsrParseVector -
*/
JsonNode jsonUcsrParseVector[] =
{
  { "/duration",                                                           duration                   },
  { "/restriction",                                                        restriction                },
  { "/restriction/attributeExpression",                                    attributeExpression        },
  { "/restriction/scopes",                                                 jsonNullTreat              },
  { "/restriction/scopes/scope",                                           scope,                     },
  { "/restriction/scopes/scope/type",                                      scopeType                  },
  { "/restriction/scopes/scope/value",                                     scopeValue                 },

  { "/restriction/scopes/scope/value/circle",                              circle                     },
  { "/restriction/scopes/scope/value/circle/centerLatitude",               circleCenterLatitude       },
  { "/restriction/scopes/scope/value/circle/centerLongitude",              circleCenterLongitude      },
  { "/restriction/scopes/scope/value/circle/radius",                       circleRadius               },
  { "/restriction/scopes/scope/value/circle/inverted",                     circleInverted             },

  { "/restriction/scopes/scope/value/polygon",                             polygon                    },
  { "/restriction/scopes/scope/value/polygon/inverted",                    polygonInverted            },
  { "/restriction/scopes/scope/value/polygon/vertices",                    polygonVertexList          },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice",            polygonVertex              },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/latitude",   polygonVertexLatitude      },
  { "/restriction/scopes/scope/value/polygon/vertices/vertice/longitude",  polygonVertexLongitude     },
  { "/restriction/scopes/scope/value/polygon/inverted",                    polygonInverted            },

  { "/subscriptionId",                                                     subscriptionId             },
  { "/notifyConditions",                                                   jsonNullTreat              },
  { "/notifyConditions/notifyCondition",                                   notifyCondition            },
  { "/notifyConditions/notifyCondition/type",                              notifyConditionType        },
  { "/notifyConditions/notifyCondition/condValues",                        jsonNullTreat              },
  { "/notifyConditions/notifyCondition/condValues/condValue",              notifyConditionCondValue   },
  { "/notifyConditions/notifyCondition/restriction",                       notifyConditionRestriction },
  { "/throttling",                                                         throttling                 },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonUcsrInit - 
*/
void jsonUcsrInit(ParseData* parseDataP)
{
  jsonUcsrRelease(parseDataP);

  parseDataP->ucsr.notifyConditionP  = NULL;
  parseDataP->ucsr.scopeP            = NULL;
  parseDataP->ucsr.res.restrictions  = 0;
  parseDataP->errorString            = "";
}



/* ****************************************************************************
*
* jsonUcsrRelease - 
*/
void jsonUcsrRelease(ParseData* parseDataP)
{
  parseDataP->ucsr.res.release();
}



/* ****************************************************************************
*
* jsonUcsrCheck - 
*/
std::string jsonUcsrCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  std::string s;
  s = parseDataP->ucsr.res.check(parseDataP->errorString, 0);
  return s;
}



/* ****************************************************************************
*
* jsonUcsrPresent - 
*/
void jsonUcsrPresent(ParseData* parseDataP)
{
  printf("jsonUcsrPresent\n");

  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

}
