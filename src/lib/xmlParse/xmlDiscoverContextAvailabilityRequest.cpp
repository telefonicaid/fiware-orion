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

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/Restriction.h"
#include "ngsi/EntityId.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "xmlParse/xmlDiscoverContextAvailabilityRequest.h"
#include "xmlParse/xmlParse.h"



/* ****************************************************************************
*
* entityId - 
*/
static int entityId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  reqDataP->dcar.entityIdP = new EntityId();
  reqDataP->dcar.res.entityIdVector.push_back(reqDataP->dcar.entityIdP);

  std::string es = entityIdParse(DiscoverContextAvailability, node, reqDataP->dcar.entityIdP);

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

  reqDataP->dcar.entityIdP->id = node->value();

  return 0;
}



/* ****************************************************************************
*
* attribute - 
*/
static int attribute(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute"));

  reqDataP->dcar.res.attributeList.push_back(node->value());

  return 0;
}



/* ****************************************************************************
*
* restriction - 
*/
static int restriction(xml_node<>* node, ParseData* reqDataP)
{
  ++reqDataP->dcar.res.restrictions;
  return 0;
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static int attributeExpression(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));

  reqDataP->dcar.res.restriction.attributeExpression.set(node->value());
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
  reqDataP->dcar.scopeP = newScopeP;
  reqDataP->dcar.res.restriction.scopeVector.push_back(reqDataP->dcar.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType - 
*/
static int scopeType(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));

  reqDataP->dcar.scopeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* scopeValue - 
*/
static int scopeValue(xml_node<>* node, ParseData* reqData)
{
  if (reqData->dcar.scopeP->type == "FIWARE_Location")
  {
    reqData->dcar.scopeP->value = "FIWARE_Location";
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqData->dcar.scopeP->type.c_str()));
  }
  else
  {
    reqData->dcar.scopeP->value = node->value();
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", node->value(), reqData->dcar.scopeP->type.c_str()));
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
  reqData->dcar.scopeP->scopeType = ScopeAreaCircle;
  return 0;
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static int circleCenterLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", node->value()));
  reqData->dcar.scopeP->circle.origin.latitude = atof(node->value());

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->dcar.scopeP->circle.origin.longitude = atof(node->value());
  return 0;
}



/* ****************************************************************************
*
* circleRadius - 
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->dcar.scopeP->circle.radius = atof(node->value());
  return 0;
}



/* ****************************************************************************
*
* polygon - 
*/
static int polygon(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygon"));
  reqData->dcar.scopeP->scopeType = ScopeAreaPolygon;
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
  reqData->dcar.vertexP = new ScopePoint();
  reqData->dcar.scopeP->polygon.vertexList.push_back(reqData->dcar.vertexP);
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLatitude - 
*/
static int polygonVertexLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", node->value()));
  reqData->dcar.vertexP->latitude = atof(node->value());
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->dcar.vertexP->longitude = atof(node->value());
  return 0;
}



/* ****************************************************************************
*
* dcarInit - 
*/
void dcarInit(ParseData* reqDataP)
{
  dcarRelease(reqDataP);

  reqDataP->dcar.entityIdP     = NULL;
  reqDataP->dcar.scopeP        = NULL;
  reqDataP->errorString        = "";

  reqDataP->dcar.res.restrictions = 0;
  reqDataP->dcar.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* dcarRelease - 
*/
void dcarRelease(ParseData* reqDataP)
{
  reqDataP->dcar.res.release();
}



/* ****************************************************************************
*
* dcarCheck - 
*/
std::string dcarCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
   return reqDataP->dcar.res.check(DiscoverContextAvailability, ciP->outFormat, "", reqDataP->errorString, reqDataP->dcar.res.restrictions);
}



#define PRINTF printf
/* ****************************************************************************
*
* dcarPresent - 
*/
void dcarPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  reqDataP->dcar.res.present("");
}



/* ****************************************************************************
*
* dcarParseVector - 
*/
XmlNode dcarParseVector[] = 
{
  { "/discoverContextAvailabilityRequest",                                             nullTreat            },

  { "/discoverContextAvailabilityRequest/entityIdList",                                nullTreat            },
  { "/discoverContextAvailabilityRequest/entityIdList/entityId",                       entityId             },
  { "/discoverContextAvailabilityRequest/entityIdList/entityId/id",                    entityIdId           },

  { "/discoverContextAvailabilityRequest/attributeList",                               nullTreat            },
  { "/discoverContextAvailabilityRequest/attributeList/attribute",                     attribute            },

  { "/discoverContextAvailabilityRequest/restriction",                                 restriction          },
  { "/discoverContextAvailabilityRequest/restriction/attributeExpression",             attributeExpression  },
  { "/discoverContextAvailabilityRequest/restriction/scope",                           nullTreat            },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope",            operationScope       },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeType",  scopeType            },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue", scopeValue           },

  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/circle",                   circle                 },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/circle/center_latitude",   circleCenterLatitude   },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/circle/center_longitude",  circleCenterLongitude  },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/circle/radius",            circleRadius           },

  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/polygon",                             polygon                 },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList",                  polygonVertexList       },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex",           polygonVertex           },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/latitude",  polygonVertexLatitude   },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/longitude", polygonVertexLongitude  },

  { "LAST", NULL }
};
