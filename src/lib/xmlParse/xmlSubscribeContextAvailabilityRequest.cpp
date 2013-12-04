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
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Metadata.h"
#include "ngsi/Restriction.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"
#include "xmlParse/xmlSubscribeContextAvailabilityRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"



/* ****************************************************************************
*
* entityId - 
*/
static int entityId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId"));

  reqData->scar.entityIdP = new EntityId();
  reqData->scar.res.entityIdVector.push_back(reqData->scar.entityIdP);

  std::string es = entityIdParse(SubscribeContextAvailability, node, reqData->scar.entityIdP);

  if (es != "OK")
    reqData->errorString = es;
  
  return 0;
}



/* ****************************************************************************
*
* entityIdId - 
*/
static int entityIdId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  reqData->scar.entityIdP->id = node->value();

  return 0;
}



/* ****************************************************************************
*
* reference - 
*/
static int reference(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a reference: '%s'", node->value()));

  reqData->scar.res.reference.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* attribute - 
*/
static int attribute(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqData->scar.res.attributeList.push_back(node->value());

  return 0;
}



/* ****************************************************************************
*
* restriction - 
*/
static int restriction(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a restriction"));
  ++reqData->scar.res.restrictions;

  return 0;
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static int attributeExpression(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));
  reqData->scar.res.restriction.attributeExpression.set(node->value());
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
  reqData->scar.scopeP = newScopeP;
  reqData->scar.res.restriction.scopeVector.push_back(reqData->scar.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType - 
*/
static int scopeType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));
  reqData->scar.scopeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* scopeValue - 
*/
static int scopeValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a scopeValue: '%s'", node->value()));
  reqData->scar.scopeP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* duration - 
*/
static int duration(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  reqData->scar.res.duration.set(node->value());

  return 0;
}


/* ****************************************************************************
*
* subscriptionId - 
*/
static int subscriptionId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", node->value()));

  reqData->scar.res.subscriptionId.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* scarInit - 
*/
void scarInit(ParseData* reqData)
{
  reqData->errorString                 = "";

  reqData->scar.entityIdP              = NULL;
  reqData->scar.scopeP                 = NULL;
  reqData->scar.restrictions           = 0;
  reqData->scar.res.restrictions       = 0;

  reqData->scar.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* scarRelease - 
*/
void scarRelease(ParseData* reqData)
{
  reqData->scar.res.release();
}



/* ****************************************************************************
*
* scarCheck - 
*/
std::string scarCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->scar.res.check(SubscribeContextAvailability, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* scarPresent - 
*/
void scarPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  reqData->scar.res.present("");
}



/* ****************************************************************************
*
* scarParseVector - 
*/
XmlNode scarParseVector[] = 
{
  { "/subscribeContextAvailabilityRequest",                                             nullTreat            },

  { "/subscribeContextAvailabilityRequest/entityIdList",                                nullTreat            },
  { "/subscribeContextAvailabilityRequest/entityIdList/entityId",                       entityId             },
  { "/subscribeContextAvailabilityRequest/entityIdList/entityId/id",                    entityIdId           },

  { "/subscribeContextAvailabilityRequest/attributeList",                               nullTreat            },
  { "/subscribeContextAvailabilityRequest/attributeList/attribute",                     attribute            },

  { "/subscribeContextAvailabilityRequest/reference",                                   reference            },

  { "/subscribeContextAvailabilityRequest/duration",                                    duration             },

  { "/subscribeContextAvailabilityRequest/restriction",                                 restriction          },
  { "/subscribeContextAvailabilityRequest/restriction/attributeExpression",             attributeExpression  },
  { "/subscribeContextAvailabilityRequest/restriction/scope",                           nullTreat            },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope",            operationScope       },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope/scopeType",  scopeType            },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope/scopeValue", scopeValue           },

  { "/subscribeContextAvailabilityRequest/subscriptionId",                              subscriptionId       },

  { "LAST", NULL }
};
