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
#include <stdio.h>
#include <string>

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
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
  if (reqData->ucsr.scopeP->type == "FIWARE_Location")
  {
    reqData->ucsr.scopeP->value = "FIWARE_Location";
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
  reqData->ucsr.scopeP->areaType = AreaCircle;
  return 0;
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static int circleCenterLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", node->value()));
  reqData->ucsr.scopeP->circle.center.latitude = node->value();

  return 0;
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static int circleCenterLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", node->value()));
  reqData->ucsr.scopeP->circle.center.longitude = node->value();
  return 0;
}



/* ****************************************************************************
*
* circleRadius - 
*/
static int circleRadius(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", node->value()));
  reqData->ucsr.scopeP->circle.radius = node->value();
  return 0;
}



/* ****************************************************************************
*
* circleInverted - 
*/
static int circleInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a circleInverted: %s", node->value()));

  parseDataP->ucsr.scopeP->circle.inverted = node->value();

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
  reqData->ucsr.scopeP->areaType = AreaPolygon;
  return 0;
}



/* ****************************************************************************
*
* polygonInverted - 
*/
static int polygonInverted(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a polygonInverted: %s", node->value()));

  parseDataP->ucsr.scopeP->polygon.inverted = node->value();
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
  reqData->ucsr.vertexP = new ScopePoint();
  reqData->ucsr.scopeP->polygon.vertexList.push_back(reqData->ucsr.vertexP);
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLatitude - 
*/
static int polygonVertexLatitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLatitude: %s", node->value()));
  reqData->ucsr.vertexP->latitude = node->value();
  return 0;
}



/* ****************************************************************************
*
* polygonVertexLongitude - 
*/
static int polygonVertexLongitude(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a polygonVertexLongitude: %s", node->value()));
  reqData->ucsr.vertexP->longitude = node->value();
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
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  reqData->ucsr.res.present("");
}



/* ****************************************************************************
*
* scrParseVector - 
*/
XmlNode ucsrParseVector[] = 
{
  { "/updateContextSubscriptionRequest", nullTreat },

  { "/updateContextSubscriptionRequest/duration", duration   },
  
  { "/updateContextSubscriptionRequest/restriction",                                  restriction          },
  { "/updateContextSubscriptionRequest/restriction/attributeExpression",              attributeExpression  },
  { "/updateContextSubscriptionRequest/restriction/scope",                            nullTreat            },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope",             operationScope       },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeType",   scopeType            },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue",  scopeValue           },

  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/circle",                              circle                  },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/circle/center_latitude",              circleCenterLatitude    },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/circle/center_longitude",             circleCenterLongitude   },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/circle/radius",                       circleRadius            },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/circle/inverted",                     circleInverted          },

  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon",                             polygon                 },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon/inverted",                    polygonInverted         },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList",                  polygonVertexList       },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex",           polygonVertex           },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/latitude",  polygonVertexLatitude   },
  { "/updateContextSubscriptionRequest/restriction/scope/operationScope/scopeValue/polygon/vertexList/vertex/longitude", polygonVertexLongitude  },

  { "/updateContextSubscriptionRequest/subscriptionId", subscriptionId   },
  
  { "/updateContextSubscriptionRequest/notifyConditions",                                          nullTreat           },
  { "/updateContextSubscriptionRequest/notifyConditions/notifyCondition",                          notifyCondition     },
  { "/updateContextSubscriptionRequest/notifyConditions/notifyCondition/type",                     notifyConditionType },
  { "/updateContextSubscriptionRequest/notifyConditions/notifyCondition/condValueList",            nullTreat           },
  { "/updateContextSubscriptionRequest/notifyConditions/notifyCondition/condValueList/condValue",  condValue           },

  { "/updateContextSubscriptionRequest/throttling", throttling },

  { "LAST", NULL }
};
