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

  if (reqDataP->dcar.entityIdP != NULL)
  {
    reqDataP->dcar.entityIdP->id = node->value();
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
static int scopeValue(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a scopeValue: '%s'", node->value()));
  reqDataP->dcar.scopeP->value = node->value();
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
  return reqDataP->dcar.res.check(DiscoverContextAvailability,
                                  ciP->outFormat,
                                  "",
                                  reqDataP->errorString,
                                  reqDataP->dcar.res.restrictions);
}




/* ****************************************************************************
*
* dcarPresent -
*/
void dcarPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

  LM_T(LmtPresent, ("\n\n"));
  reqDataP->dcar.res.present("");
}



/* ****************************************************************************
*
* dcarParseVector -
*/
XmlNode dcarParseVector[] =
{
  { "/discoverContextAvailabilityRequest",                                                nullTreat           },

  { "/discoverContextAvailabilityRequest/entityIdList",                                   nullTreat           },
  { "/discoverContextAvailabilityRequest/entityIdList/entityId",                          entityId            },
  { "/discoverContextAvailabilityRequest/entityIdList/entityId/id",                       entityIdId          },

  { "/discoverContextAvailabilityRequest/attributeList",                                  nullTreat           },
  { "/discoverContextAvailabilityRequest/attributeList/attribute",                        attribute           },

  { "/discoverContextAvailabilityRequest/restriction",                                    restriction         },
  { "/discoverContextAvailabilityRequest/restriction/attributeExpression",                attributeExpression },
  { "/discoverContextAvailabilityRequest/restriction/scope",                              nullTreat           },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope",               operationScope      },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeType",     scopeType           },
  { "/discoverContextAvailabilityRequest/restriction/scope/operationScope/scopeValue",    scopeValue          },

  { "LAST", NULL }
};
