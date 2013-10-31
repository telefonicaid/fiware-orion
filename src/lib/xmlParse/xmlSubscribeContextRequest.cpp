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
  reqData->scr.entityIdP->id = node->value();
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
  LM_T(LmtParse, ("Got a scopeValue: '%s'", node->value()));
  reqData->scr.scopeP->value = node->value();
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
  if (!lmTraceIsSet(LmtDump))
    return;

  reqData->scr.res.present("");
}



/* ****************************************************************************
*
* scrParseVector - 
*/
XmlNode scrParseVector[] = 
{
  { "/subscribeContextRequest", nullTreat },

  { "/subscribeContextRequest/entityIdList",                     nullTreat         },
  { "/subscribeContextRequest/entityIdList/entityId",            entityId          },
  { "/subscribeContextRequest/entityIdList/entityId/id",         entityIdId        },

  { "/subscribeContextRequest/attributeList",            nullTreat },
  { "/subscribeContextRequest/attributeList/attribute",  attribute },

  { "/subscribeContextRequest/reference", reference },
  { "/subscribeContextRequest/duration",  duration  },

  { "/subscribeContextRequest/restriction",                                  restriction          },
  { "/subscribeContextRequest/restriction/attributeExpression",              attributeExpression  },
  { "/subscribeContextRequest/restriction/scope",                            nullTreat            },
  { "/subscribeContextRequest/restriction/scope/operationScope",             operationScope       },
  { "/subscribeContextRequest/restriction/scope/operationScope/scopeType",   scopeType            },
  { "/subscribeContextRequest/restriction/scope/operationScope/scopeValue",  scopeValue           },

  { "/subscribeContextRequest/notifyConditions",                                          nullTreat                  },
  { "/subscribeContextRequest/notifyConditions/notifyCondition",                          notifyCondition            },
  { "/subscribeContextRequest/notifyConditions/notifyCondition/restriction",              notifyConditionRestriction },
  { "/subscribeContextRequest/notifyConditions/notifyCondition/type",                     notifyConditionType        },
  { "/subscribeContextRequest/notifyConditions/notifyCondition/condValueList",            nullTreat                  },
  { "/subscribeContextRequest/notifyConditions/notifyCondition/condValueList/condValue",  condValue                  },

  { "/subscribeContextRequest/throttling", throttling },

  { "LAST", NULL }
};
