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
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUpdateContextSubscriptionRequest.h"



/* ****************************************************************************
*
* restriction -
*/
static int restriction(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a restriction"));
  ++reqData->ucsr.res.restrictions;

  return 0;
}



/* ****************************************************************************
*
* attributeExpression -
*/
static int attributeExpression(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));
  reqData->ucsr.res.restriction.attributeExpression.set(node->value());

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
  reqData->ucsr.scopeP = newScopeP;
  reqData->ucsr.res.restriction.scopeVector.push_back(reqData->ucsr.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType -
*/
static int scopeType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));
  reqData->ucsr.scopeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* scopeValue -
*/
static int scopeValue(xml_node<>* node, ParseData* reqData)
{
  if (reqData->ucsr.scopeP->type == FIWARE_LOCATION || reqData->ucsr.scopeP->type == FIWARE_LOCATION)
  {
    //
    // If the scope type is FIWARE_LOCATION (or its deprecated variant), then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    reqData->ucsr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqData->ucsr.scopeP->type.c_str()));
  }
  else
  {
    reqData->ucsr.scopeP->value = node->value();
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", node->value(), reqData->ucsr.scopeP->type.c_str()));
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
  reqData->ucsr.scopeP->areaType = orion::CircleType;
  return 0;
}



/* ****************************************************************************
*
* circleCenterLatitude -
*/
static int circleCenterLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", node->value()));
  reqData->ucsr.scopeP->circle.center.latitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude -
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->ucsr.scopeP->circle.center.longitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* circleRadius -
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->ucsr.scopeP->circle.radiusSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* circleInverted -
*/
static int circleInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", node->value()));

  parseDataP->ucsr.scopeP->circle.invertedSet(node->value());

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
  reqData->ucsr.scopeP->areaType = orion::PolygonType;
  return 0;
}



/* ****************************************************************************
*
* polygonInverted -
*/
static int polygonInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", node->value()));

  parseDataP->ucsr.scopeP->polygon.invertedSet(node->value());
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
  reqData->ucsr.vertexP = new orion::Point();
  reqData->ucsr.scopeP->polygon.vertexAdd(reqData->ucsr.vertexP);
  // reqData->ucsr.scopeP->polygon.vertexList.push_back(reqData->ucsr.vertexP);
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLatitude -
*/
static int polygonVertexLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", node->value()));
  reqData->ucsr.vertexP->latitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude -
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->ucsr.vertexP->longitudeSet(node->value());
  return 0;
}



/* ****************************************************************************
*
* duration -
*/
static int duration(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  reqData->ucsr.res.duration.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* notifyCondition -
*/
static int notifyCondition(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Notify Condition"));
  reqData->ucsr.notifyConditionP = new NotifyCondition();
  reqData->ucsr.res.notifyConditionVector.push_back(reqData->ucsr.notifyConditionP);
  return 0;
}



/* ****************************************************************************
*
* notifyConditionType -
*/
static int notifyConditionType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Notify Condition Type: '%s'", node->value()));
  reqData->ucsr.notifyConditionP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* condValue -
*/
static int condValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a Cond Value: '%s'", node->value()));
  reqData->ucsr.notifyConditionP->condValueList.push_back(node->value());
  return 0;
}



/* ****************************************************************************
*
* subscriptionId -
*/
static int subscriptionId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", node->value()));
  reqData->ucsr.res.subscriptionId.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* throttling -
*/
static int throttling(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a throttling: '%s'", node->value()));
  reqData->ucsr.res.throttling.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* ucsrInit -
*/
void ucsrInit(ParseData* reqData)
{
  reqData->ucsr.notifyConditionP       = NULL;
  reqData->ucsr.scopeP                 = NULL;
  reqData->ucsr.res.restrictions       = 0;
}



/* ****************************************************************************
*
* ucsrRelease -
*/
void ucsrRelease(ParseData* reqData)
{
  reqData->ucsr.res.release();
}



/* ****************************************************************************
*
* ucsrCheck -
*/
std::string ucsrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->ucsr.res.check(UpdateContextSubscription, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* ucsrPresent -
*/
void ucsrPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  LM_T(LmtPresent, ("\n\n"));
  reqData->ucsr.res.present("");
}



/* ****************************************************************************
*
* scrParseVector -
*/
#define UCS   "/updateContextSubscriptionRequest"
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

XmlNode ucsrParseVector[] =
{
  { "/updateContextSubscriptionRequest",         nullTreat                   },

  { UCS "/duration",                             duration                    },

  { UCS "/restriction",                          restriction                 },
  { UCS RS "/attributeExpression",               attributeExpression         },
  { UCS RS "/scope",                             nullTreat                   },
  { UCS RS SC "/operationScope",                 operationScope              },
  { UCS RS SC OSC "/scopeType",                  scopeType                   },
  { UCS RS SC OSC "/scopeValue",                 scopeValue                  },

  { UCS RS SC OSC SVAL "/circle",                circle                      },
  { UCS RS SC OSC SVAL CRC "/centerLatitude",    circleCenterLatitude        },
  { UCS RS SC OSC SVAL CRC "/centerLongitude",   circleCenterLongitude       },
  { UCS RS SC OSC SVAL CRC "/radius",            circleRadius                },
  { UCS RS SC OSC SVAL CRC "/inverted",          circleInverted              },

  { UCS RS SC OSC SVAL "/polygon",               polygon                     },
  { UCS RS SC OSC SVAL POL "/inverted",          polygonInverted             },
  { UCS RS SC OSC SVAL POL "/vertexList",        polygonVertexList           },
  { UCS RS SC OSC SVAL POL VXL "/vertex",        polygonVertex               },
  { UCS RS SC OSC SVAL POL VXL VX "/latitude",   polygonVertexLatitude       },
  { UCS RS SC OSC SVAL POL VXL VX "/longitude",  polygonVertexLongitude      },

  { UCS "/subscriptionId",                       subscriptionId              },

  { UCS "/notifyConditions",                     nullTreat                   },
  { UCS NCL "/notifyCondition",                  notifyCondition             },
  { UCS NCL NC "/type",                          notifyConditionType         },
  { UCS NCL NC "/condValueList",                 nullTreat                   },
  { UCS NCL NC CVL "/condValue",                 condValue                   },

  { UCS "/throttling",                           throttling                  },

  { "LAST", NULL }
};
