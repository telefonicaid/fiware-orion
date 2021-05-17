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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonDiscoverContextAvailabilityRequest.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "parse/nullTreat.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->dcar.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", reqDataP->dcar.entityIdP));
  reqDataP->dcar.entityIdP->id        = "";
  reqDataP->dcar.entityIdP->type      = "";
  reqDataP->dcar.entityIdP->isPattern = "false";

  reqDataP->dcar.res.entityIdVector.push_back(reqDataP->dcar.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->dcar.entityIdP->id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->dcar.entityIdP->id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->dcar.entityIdP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->dcar.entityIdP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  reqDataP->dcar.entityIdP->isPattern = value;

  if (!isTrue(value) && !isFalse(value))
  {
    return "invalid isPattern value for entity: /" + value + "/";
  }

  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute: '%s'", value.c_str()));

  reqDataP->dcar.res.attributeList.push_back(value);

  return "OK";
}



/* ****************************************************************************
*
* attributeList - 
*/
static std::string attributeList(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeList: '%s'", value.c_str()));
  return "OK";
}



/* ****************************************************************************
*
* restriction - 
*/
static std::string restriction(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->dcar.res.restrictions += 1;
  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  reqDataP->dcar.res.restriction.attributeExpression.set(value);

  if (value.empty())
  {
    alarmMgr.badInput(clientIp, "empty attribute expression");
    return "Empty attribute expression";
  }

  return "OK";
}



/* ****************************************************************************
*
* operationScope - 
*/
static std::string operationScope(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an operationScope"));

  reqDataP->dcar.scopeP = new Scope();
  reqDataP->dcar.res.restriction.scopeVector.push_back(reqDataP->dcar.scopeP);
  reqDataP->dcar.scopeP->type  = "";
  reqDataP->dcar.scopeP->value = "";

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->dcar.scopeP->type = value;
  LM_T(LmtParse, ("Set scope 'type' to '%s' for a scope", reqDataP->dcar.scopeP->type.c_str()));
  return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->dcar.scopeP->value = value;
  LM_T(LmtParse, ("Set scope 'value' to '%s' for a scope", reqDataP->dcar.scopeP->value.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* jsonDcarInit - 
*/
void jsonDcarInit(ParseData* reqDataP)
{
  jsonDcarRelease(reqDataP);

  reqDataP->dcar.entityIdP     = NULL;
  reqDataP->dcar.scopeP        = NULL;
  reqDataP->errorString        = "";

  reqDataP->dcar.res.restrictions = 0;
  reqDataP->dcar.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* jsonDcarRelease - 
*/
void jsonDcarRelease(ParseData* reqDataP)
{
  reqDataP->dcar.res.release();
}



/* ****************************************************************************
*
* jsonDcarCheck - 
*/
std::string jsonDcarCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
  return reqDataP->dcar.res.check(reqDataP->errorString);
}




/* ****************************************************************************
*
* dcarParseVector - 
*/
JsonNode jsonDcarParseVector[] =
{
  { "/entities",                          jsonNullTreat         },
  { "/entities/entity",                   entityId              },
  { "/entities/entity/id",                entityIdId            },
  { "/entities/entity/type",              entityIdType          },
  { "/entities/entity/isPattern",         entityIdIsPattern     },

  { "/attributes",                        attributeList         },
  { "/attributes/attribute",              attribute             },

  { "/restriction",                       restriction           },
  { "/restriction/attributeExpression",   attributeExpression   },
  { "/restriction/scopes",                jsonNullTreat         },
  { "/restriction/scopes/scope",          operationScope        },
  { "/restriction/scopes/scope/type",     scopeType             },
  { "/restriction/scopes/scope/value",    scopeValue            },

  { "LAST", NULL }
};
