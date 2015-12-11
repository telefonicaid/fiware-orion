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
#include "alarmMgr/alarmMgr.h"
#include "orionTypes/areas.h"
#include "ngsi/Request.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Restriction.h"
#include "ngsi10/QueryContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlQueryContextRequest.h"
#include "xmlParse/xmlParse.h"



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  reqDataP->qcr.entityIdP = new EntityId();
  reqDataP->qcr.res.entityIdVector.push_back(reqDataP->qcr.entityIdP);

  std::string es = entityIdParse(QueryContext, node, reqDataP->qcr.entityIdP);

  if (es != "OK")
  {
    reqDataP->errorString = es;
  }

  return 0;
}



/* ****************************************************************************
*
* entityIdId -
*/
static int entityIdId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  if (reqDataP->qcr.entityIdP != NULL)
  {
    reqDataP->qcr.entityIdP->id = node->value();
  }
  else
  {
    alarmMgr.badInput(clientIp, "XML parse error");
    reqDataP->errorString = "Bad Input (XML parse error)";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* attribute -
*/
static int attribute(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqDataP->qcr.res.attributeList.push_back(node->value());

  return 0;
}



/* ****************************************************************************
*
* restriction -
*/
static int restriction(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a restriction"));

  LM_T(LmtRestriction, ("Got a restriction - from value %d", reqDataP->qcr.res.restrictions));
  ++reqDataP->qcr.res.restrictions;
  LM_T(LmtRestriction, ("Got a restriction - to value %d", reqDataP->qcr.res.restrictions));

  return 0;
}



/* ****************************************************************************
*
* attributeExpression -
*/
static int attributeExpression(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));
  reqDataP->qcr.res.restriction.attributeExpression.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* operationScope -
*/
static int operationScope(xml_node<>* node, ParseData* reqDataP)
{
  Scope* newScopeP = new Scope();

  LM_T(LmtParse, ("Got an operationScope"));
  reqDataP->qcr.scopeP = newScopeP;
  reqDataP->qcr.res.restriction.scopeVector.push_back(reqDataP->qcr.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType -
*/
static int scopeType(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));
  reqDataP->qcr.scopeP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* scopeValue -
*/
static int scopeValue(xml_node<>* node, ParseData* reqData)
{
  if (reqData->qcr.scopeP->type == FIWARE_LOCATION || reqData->qcr.scopeP->type == FIWARE_LOCATION_DEPRECATED)
  {
    //
    // If the scope type is FIWARE_LOCATION (or its deprecated variant), then the value of this scope is stored in 'circle' or 'polygon'.
    // The field 'value' is not used as more complexity is needed.
    // scopeP->value is here set to FIWARE_LOCATION, in an attempt to warn a future use of 'scopeP->value' when
    // instead 'circle' or 'polygon' should be used.
    //
    reqData->qcr.scopeP->value = FIWARE_LOCATION;
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqData->qcr.scopeP->type.c_str()));
  }
  else
  {
    reqData->qcr.scopeP->value = node->value();
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", node->value(), reqData->qcr.scopeP->type.c_str()));
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
  reqData->qcr.scopeP->areaType = orion::CircleType;

  return 0;
}



/* ****************************************************************************
*
* circleCenterLatitude -
*/
static int circleCenterLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", node->value()));
  reqData->qcr.scopeP->circle.center.latitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude -
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->qcr.scopeP->circle.center.longitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleRadius -
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->qcr.scopeP->circle.radiusSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleInverted -
*/
static int circleInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", node->value()));

  parseDataP->qcr.scopeP->circle.invertedSet(node->value());

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
  reqData->qcr.scopeP->areaType = orion::PolygonType;

  return 0;
}



/* ****************************************************************************
*
* polygonInverted -
*/
static int polygonInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", node->value()));

  parseDataP->qcr.scopeP->polygon.invertedSet(node->value());
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

  reqData->qcr.vertexP = new orion::Point();
  reqData->qcr.scopeP->polygon.vertexList.push_back(reqData->qcr.vertexP);

  return 0;
}



/* ****************************************************************************
*
* polygonVertexLatitude -
*/
static int polygonVertexLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", node->value()));
  reqData->qcr.vertexP->latitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude -
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->qcr.vertexP->longitudeSet(node->value());

  return 0;
}



/* ****************************************************************************
*
* qcrInit -
*/
void qcrInit(ParseData* reqDataP)
{
  qcrRelease(reqDataP);

  reqDataP->qcr.entityIdP            = NULL;
  reqDataP->qcr.scopeP               = NULL;
  reqDataP->errorString              = "";

  reqDataP->qcr.res.restrictions     = 0;
  reqDataP->qcr.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* qcrRelease -
*/
void qcrRelease(ParseData* reqDataP)
{
  reqDataP->qcr.res.release();
}



/* ****************************************************************************
*
* qcrCheck -
*/
std::string qcrCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
  return reqDataP->qcr.res.check(ciP, QueryContext, "", reqDataP->errorString, 0);
}



/* ****************************************************************************
*
* qcrPresent -
*/
void qcrPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

  LM_T(LmtPresent, ("\n\n"));
  reqDataP->qcr.res.present("");
}



/* ****************************************************************************
*
* qcrParseVector -
*/
#define QCR  "/queryContextRequest"
#define EL   "/entityIdList"
#define AL   "/attributeList"
#define RS   "/restriction"
#define SC   "/scope"
#define OSC  "/operationScope"
#define SV   "/scopeValue"
#define CRC  "/circle"
#define POL  "/polygon"
#define VXL  "/vertexList"
#define VX   "/vertex"

XmlNode qcrParseVector[] =
{
  { "/queryContextRequest",                       nullTreat               },
  { QCR "/entityIdList",                          nullTreat               },
  { QCR EL "/entityId",                           entityId                },
  { QCR EL "/entityId/id",                        entityIdId              },
  { QCR "/attributeList",                         nullTreat               },
  { QCR AL "/attribute",                          attribute               },
  { QCR "/restriction",                           restriction             },
  { QCR RS "/attributeExpression",                attributeExpression     },
  { QCR RS "/scope",                              nullTreat               },
  { QCR RS SC "/operationScope",                  operationScope          },
  { QCR RS SC OSC "/scopeType",                   scopeType               },
  { QCR RS SC OSC "/scopeValue",                  scopeValue              },

  { QCR RS SC OSC SV "/circle",                   circle                  },
  { QCR RS SC OSC SV CRC "/centerLatitude",       circleCenterLatitude    },
  { QCR RS SC OSC SV CRC "/centerLongitude",      circleCenterLongitude   },
  { QCR RS SC OSC SV CRC "/radius",               circleRadius            },
  { QCR RS SC OSC SV CRC "/inverted",             circleInverted          },

  { QCR RS SC OSC SV "/polygon",                  polygon                 },
  { QCR RS SC OSC SV POL "/inverted",             polygonInverted         },
  { QCR RS SC OSC SV POL "/vertexList",           polygonVertexList       },
  { QCR RS SC OSC SV POL VXL "/vertex",           polygonVertex           },
  { QCR RS SC OSC SV POL VXL VX "/latitude",      polygonVertexLatitude   },
  { QCR RS SC OSC SV POL VXL VX "/longitude",     polygonVertexLongitude  },

  { "LAST", NULL }
};
