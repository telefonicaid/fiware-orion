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
#include "common/wsStrip.h"
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
static int entityId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  reqDataP->scar.entityIdP = new EntityId();
  reqDataP->scar.res.entityIdVector.push_back(reqDataP->scar.entityIdP);

  std::string es = entityIdParse(SubscribeContextAvailability, node, reqDataP->scar.entityIdP);

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

  if (reqDataP->scar.entityIdP != NULL)
  {
    reqDataP->scar.entityIdP->id = node->value();
  }
  else
  {
    LM_W(("Bad Input (XML parse error)"));
    reqDataP->errorString = "Bad Input (XML parse error)";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* reference -
*/
static int reference(xml_node<>* node, ParseData* reqDataP)
{
  std::string  ref      = node->value();
  char*        stripped = wsStrip((char*) ref.c_str());

  LM_T(LmtParse, ("Got a reference: '%s'", stripped));
  reqDataP->scar.res.reference.set(stripped);

  return 0;
}



/* ****************************************************************************
*
* attribute -
*/
static int attribute(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqDataP->scar.res.attributeList.push_back(node->value());

  return 0;
}



/* ****************************************************************************
*
* restriction -
*/
static int restriction(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a restriction"));
  ++reqDataP->scar.res.restrictions;

  return 0;
}



/* ****************************************************************************
*
* attributeExpression -
*/
static int attributeExpression(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", node->value()));
  reqDataP->scar.res.restriction.attributeExpression.set(node->value());

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
  reqDataP->scar.scopeP = newScopeP;
  reqDataP->scar.res.restriction.scopeVector.push_back(reqDataP->scar.scopeP);

  return 0;
}



/* ****************************************************************************
*
* scopeType -
*/
static int scopeType(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scopeType: '%s'", node->value()));
  reqDataP->scar.scopeP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* scopeValue -
*/
static int scopeValue(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scopeValue: '%s'", node->value()));
  reqDataP->scar.scopeP->value = node->value();

  return 0;
}



/* ****************************************************************************
*
* duration -
*/
static int duration(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  reqDataP->scar.res.duration.set(node->value());

  return 0;
}


/* ****************************************************************************
*
* scarInit -
*/
void scarInit(ParseData* reqDataP)
{
  scarRelease(reqDataP);

  reqDataP->scar.entityIdP              = NULL;
  reqDataP->scar.scopeP                 = NULL;
  reqDataP->errorString                 = "";

  reqDataP->scar.res.restrictions       = 0;
  reqDataP->scar.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* scarRelease -
*/
void scarRelease(ParseData* reqDataP)
{
  reqDataP->scar.res.release();
}



/* ****************************************************************************
*
* scarCheck -
*/
std::string scarCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
  return reqDataP->scar.res.check(SubscribeContextAvailability, ciP->outFormat, "", reqDataP->errorString, 0);
}



/* ****************************************************************************
*
* scarPresent -
*/
void scarPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

  reqDataP->scar.res.present("");
}



/* ****************************************************************************
*
* scarParseVector -
*/
XmlNode scarParseVector[] =
{
  { "/subscribeContextAvailabilityRequest",                                                nullTreat            },

  { "/subscribeContextAvailabilityRequest/entityIdList",                                   nullTreat            },
  { "/subscribeContextAvailabilityRequest/entityIdList/entityId",                          entityId             },
  { "/subscribeContextAvailabilityRequest/entityIdList/entityId/id",                       entityIdId           },

  { "/subscribeContextAvailabilityRequest/attributeList",                                  nullTreat            },
  { "/subscribeContextAvailabilityRequest/attributeList/attribute",                        attribute            },

  { "/subscribeContextAvailabilityRequest/reference",                                      reference            },

  { "/subscribeContextAvailabilityRequest/duration",                                       duration             },

  { "/subscribeContextAvailabilityRequest/restriction",                                    restriction          },
  { "/subscribeContextAvailabilityRequest/restriction/attributeExpression",                attributeExpression  },
  { "/subscribeContextAvailabilityRequest/restriction/scope",                              nullTreat            },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope",               operationScope       },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope/scopeType",     scopeType            },
  { "/subscribeContextAvailabilityRequest/restriction/scope/operationScope/scopeValue",    scopeValue           },

  { "LAST", NULL }
};
