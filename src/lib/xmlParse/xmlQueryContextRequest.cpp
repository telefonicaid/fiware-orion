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

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/areas.h"
#include "ngsi/Request.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Restriction.h"
#include "ngsi10/QueryContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlQueryContextRequest.h"
#include "xmlParse/xmlParse.h"

using namespace orion;



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
    reqDataP->errorString = es;

  return 0;
}



/* ****************************************************************************
*
* entityIdId - 
*/
static int entityIdId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  reqDataP->qcr.entityIdP->id = node->value();

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
  if (reqData->qcr.scopeP->type == FIWARE_LOCATION)
  {
    //
    // If the scope type is 'FIWARE_Location', then the value of this scope is stored in 'circle' or 'polygon'.
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
  reqData->qcr.scopeP->circle.center.latitude = node->value();

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->qcr.scopeP->circle.center.longitude = node->value();
  return 0;
}



/* ****************************************************************************
*
* circleRadius - 
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->qcr.scopeP->circle.radius = node->value();
  return 0;
}



/* ****************************************************************************
*
* circleInverted - 
*/
static int circleInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", node->value()));

  parseDataP->qcr.scopeP->circle.inverted = node->value();

  if (!isTrue(node->value()) && !isFalse(node->value()))
  {
    parseDataP->errorString = std::string("bad string for circle/inverted: '") + node->value() + "'";
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

  parseDataP->qcr.scopeP->polygon.inverted = node->value();
  if (!isTrue(node->value()) && !isFalse(node->value()))
  {
    parseDataP->errorString = std::string("bad string for polygon/inverted: '") + node->value() + "'";
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
  reqData->qcr.vertexP->latitude = node->value();
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->qcr.vertexP->longitude = node->value();
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
  return reqDataP->qcr.res.check(QueryContext, ciP->outFormat, "", reqDataP->errorString, 0);
}


#define PRINTF printf
/* ****************************************************************************
*
* qcrPresent - 
*/
void qcrPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  reqDataP->qcr.res.present("");
}



/* ****************************************************************************
*
* qcrParseVector - 
*/
XmlNode qcrParseVector[] = 
{
  { "/queryContextRequest",                                             nullTreat            },
  { "/queryContextRequest/entityIdList",                                nullTreat            },
  { "/queryContextRequest/entityIdList/entityId",                       entityId             },
  { "/queryContextRequest/entityIdList/entityId/id",                    entityIdId           },
  { "/queryContextRequest/attributeList",                               nullTreat            },
  { "/queryContextRequest/attributeList/attribute",                     attribute            },
  { "/queryContextRequest/restriction",                                 restriction          },
  { "/queryContextRequest/restriction/attributeExpression",             attributeExpression  },
  { "/queryContextRequest/restriction/scope",                           nullTreat            },
  { "/queryContextRequest/restriction/scope/operationScope",            operationScope       },
  { "/queryContextRequest/restriction/scope/operationScope/scopeType",  scopeType            },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue", scopeValue           },

  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/circle",                              circle                  },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/circle/center_latitude",              circleCenterLatitude    },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/circle/center_longitude",             circleCenterLongitude   },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/circle/radius",                       circleRadius            },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/circle/inverted",                     circleInverted          },

  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon",                             polygon                 },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon/inverted",                    polygonInverted         },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList",                  polygonVertexList       },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex",           polygonVertex           },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/latitude",  polygonVertexLatitude   },
  { "/queryContextRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/longitude", polygonVertexLongitude  },

  { "LAST", NULL }
};
