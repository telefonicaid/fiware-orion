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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/areas.h"
#include "ngsi/ParseData.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Metadata.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlSubscribeContextRequest.h"



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId"));

  reqData->scr.entityIdP = new EntityId();
  reqData->scr.res.entityIdVector.push_back(reqData->scr.entityIdP);

  std::string es = entityIdParse(SubscribeContext, node, reqData->scr.entityIdP);

  if (es != "OK")
  {
    reqData->errorString = es;
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* entityIdId -
*/
static int entityIdId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  if (reqData->scr.entityIdP != NULL)
  {
    reqData->scr.entityIdP->id = node->value();
  }
  else
  {
    LM_W(("Bad Input (XML parse error)"));
    reqData->errorString = "Bad Input (XML parse error)";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* attribute -
*/
static int attribute(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqData->scr.res.attributeList.push_back(node->value());

  return 0;
}



/* ****************************************************************************
*
* restriction -
*/
static int restriction(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a restriction"));
  ++reqData->scr.res.restrictions;

  return 0;
}



/* ****************************************************************************
*
* attributeExpression -
*/
static int attributeExpression(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));
  reqData->scr.res.restriction.attributeExpression.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* operationScope -
*/
static int operationScope(xml_node<>* node, ParseData* reqData)
{
  Scope* newScopeP = new Scope();

  LM_T(LmtParse, ("Got an operationScope"));
  reqData->scr.scopeP = newScopeP;
  reqData->scr.res.restriction.scopeVector.push_back(reqData->scr.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType -
*/
static int scopeType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));
  reqData->scr.scopeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* scopeValue -
*/
static int scopeValue(xml_node<>* node, ParseData* reqData)
{
  if (reqData->scr.scopeP->type == FIWARE_LOCATION || reqData->scr.scopeP->type == FIWARE_LOCATION_DEPRECATED)
  {
    //
    // If the scope type is FIWARE_LOCATION (or its deprecated variant), then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    reqData->scr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqData->scr.scopeP->type.c_str()));
  }
  else
  {
    reqData->scr.scopeP->value = node->value();
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", node->value(), reqData->scr.scopeP->type.c_str()));
  }

  return 0;
}



/* ****************************************************************************
*
* circle -
*/
static int circle(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circle"));
  reqData->scr.scopeP->areaType = orion::CircleType;
  return 0;
}



/* ****************************************************************************
*
* circleCenterLatitude -
*/
static int circleCenterLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", node->value()));
  reqData->scr.scopeP->circle.center.latitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude -
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->scr.scopeP->circle.center.longitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* circleRadius -
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->scr.scopeP->circle.radiusSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* circleInverted -
*/
static int circleInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", node->value()));

  parseDataP->scr.scopeP->circle.invertedSet(std::string(node->value()));

  if (!isTrue(node->value()) && !isFalse(node->value()))
  {
    parseDataP->errorString = std::string("bad string for circle/inverted: /") + node->value() + "/";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* polygon -
*/
static int polygon(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygon"));
  reqData->scr.scopeP->areaType = orion::PolygonType;
  return 0;
}



/* ****************************************************************************
*
* polygonInverted -
*/
static int polygonInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", node->value()));

  parseDataP->scr.scopeP->polygon.invertedSet(std::string(node->value()));

  if (!isTrue(node->value()) && !isFalse(node->value()))
  {
    parseDataP->errorString = std::string("bad string for polygon/inverted: /") + node->value() + "/";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* polygonVertexList -
*/
static int polygonVertexList(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexList"));
  return 0;
}



/* ****************************************************************************
*
* polygonVertex -
*/
static int polygonVertex(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertex - creating new vertex for the vertex list"));
  reqData->scr.vertexP = new orion::Point();
  reqData->scr.scopeP->polygon.vertexAdd(reqData->scr.vertexP);
  // reqData->scr.scopeP->polygon.vertexList.push_back(reqData->scr.vertexP);
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLatitude -
*/
static int polygonVertexLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", node->value()));
  reqData->scr.vertexP->latitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude -
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->scr.vertexP->longitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* reference -
*/
static int reference(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a reference: '%s'", node->value()));
  reqData->scr.res.reference.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* duration -
*/
static int duration(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  reqData->scr.res.duration.set(node->value());
  return 0;
}


/* ****************************************************************************
*
* notifyCondition -
*/
static int notifyCondition(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Notify Condition"));
  reqData->scr.notifyConditionP = new NotifyCondition();
  reqData->scr.res.notifyConditionVector.push_back(reqData->scr.notifyConditionP);
  return 0;
}



/* ****************************************************************************
*
* notifyConditionRestriction -
*/
static int notifyConditionRestriction(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Notify Condition restriction"));

  reqData->scr.notifyConditionP->restriction.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* notifyConditionType -
*/
static int notifyConditionType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Notify Condition Type: '%s'", node->value()));
  reqData->scr.notifyConditionP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* condValue -
*/
static int condValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Cond Value: '%s'", node->value()));
  reqData->scr.notifyConditionP->condValueList.push_back(node->value());
  return 0;
}



/* ****************************************************************************
*
* throttling -
*/
static int throttling(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a throttling: '%s'", node->value()));
  reqData->scr.res.throttling.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* scrInit -
*/
void scrInit(ParseData* reqData)
{
  reqData->scr.entityIdP              = NULL;
  reqData->scr.attributeMetadataP     = NULL;
  reqData->scr.restrictionP           = NULL;
  reqData->scr.notifyConditionP       = NULL;
  reqData->scr.scopeP                 = NULL;
  reqData->scr.res.restrictions       = 0;
  reqData->errorString                = "";
}



/* ****************************************************************************
*
* scrRelease -
*/
void scrRelease(ParseData* reqData)
{
  reqData->scr.res.release();
}



/* ****************************************************************************
*
* scrCheck -
*/
std::string scrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->scr.res.check(SubscribeContext, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* scrPresent -
*/
void scrPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  reqData->scr.res.present("");
}



/* ****************************************************************************
*
* scrParseVector -
*/
#define SCR   "/subscribeContextRequest"
#define EIDL  "/entityIdList"
#define EID   "/entityId"
#define ATTRL "/attributeList"
#define RS    "/restriction"
#define SC    "/scope"
#define OSC   "/operationScope"
#define SVAL  "/scopeValue"
#define CRC   "/circle"
#define POL   "/polygon"
#define VXL   "/vertexList"
#define VX    "/vertex"
#define NCL   "/notifyConditions"
#define NC    "/notifyCondition"
#define CVL   "/condValueList"

XmlNode scrParseVector[] =
{
  { "/subscribeContextRequest",                    nullTreat                  },

  { SCR "/entityIdList",                           nullTreat                  },
  { SCR EIDL "/entityId",                          entityId                   },
  { SCR EIDL EID "/id",                            entityIdId                 },

  { SCR "/attributeList",                          nullTreat                  },
  { SCR ATTRL "/attribute",                        attribute                  },

  { SCR "/reference",                              reference                  },
  { SCR "/duration",                               duration                   },

  { SCR "/restriction",                            restriction                },
  { SCR RS "/attributeExpression",                 attributeExpression        },
  { SCR RS "/scope",                               nullTreat                  },
  { SCR RS SC "/operationScope",                   operationScope             },
  { SCR RS SC OSC "/scopeType",                    scopeType                  },
  { SCR RS SC OSC "/scopeValue",                   scopeValue                 },

  { SCR RS SC OSC SVAL "/circle",                  circle                     },
  { SCR RS SC OSC SVAL CRC "/centerLatitude",      circleCenterLatitude       },
  { SCR RS SC OSC SVAL CRC "/centerLongitude",     circleCenterLongitude      },
  { SCR RS SC OSC SVAL CRC "/radius",              circleRadius               },
  { SCR RS SC OSC SVAL CRC "/inverted",            circleInverted             },

  { SCR RS SC OSC SVAL "/polygon",                 polygon                    },
  { SCR RS SC OSC SVAL POL "/inverted",            polygonInverted            },
  { SCR RS SC OSC SVAL POL "/vertexList",          polygonVertexList          },
  { SCR RS SC OSC SVAL POL VXL "/vertex",          polygonVertex              },
  { SCR RS SC OSC SVAL POL VXL VX "/latitude",     polygonVertexLatitude      },
  { SCR RS SC OSC SVAL POL VXL VX "/longitude",    polygonVertexLongitude     },

  { SCR "/notifyConditions",                       nullTreat                  },
  { SCR NCL "/notifyCondition",                    notifyCondition            },
  { SCR NCL NC "/restriction",                     notifyConditionRestriction },
  { SCR NCL NC "/type",                            notifyConditionType        },
  { SCR NCL NC "/condValueList",                   nullTreat                  },
  { SCR NCL NC CVL "/condValue",                   condValue                  },

  { SCR "/throttling", throttling },

  { "LAST", NULL }
};
