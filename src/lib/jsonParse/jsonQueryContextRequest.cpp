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

#include "common/globals.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonQueryContextRequest.h"
#include "jsonParse/jsonNullTreat.h"

#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->qcr.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", reqDataP->qcr.entityIdP));
  reqDataP->qcr.entityIdP->id        = "not in use";
  reqDataP->qcr.entityIdP->type      = "not in use";
  reqDataP->qcr.entityIdP->isPattern = "false";

  reqDataP->qcr.res.entityIdVector.push_back(reqDataP->qcr.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->qcr.entityIdP->id = value;
   LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->qcr.entityIdP->id.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->qcr.entityIdP->type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->qcr.entityIdP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  reqDataP->qcr.entityIdP->isPattern = value;

  if (!isTrue(value) && !isFalse(value))
    return "bad 'isPattern' value: '" + value + "'";

  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attribute: '%s'", value.c_str()));

  if (value == "")
  {
    reqDataP->errorString = "Empty attribute name";
    LM_W(("Empty attribute name"));
  }

  reqDataP->qcr.res.attributeList.push_back(value);

  return "OK";
}



/* ****************************************************************************
*
* attributeList - 
*/
static std::string attributeList(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeList: '%s'", value.c_str()));
  return "OK";
}



/* ****************************************************************************
*
* restriction - 
*/
static std::string restriction(std::string path, std::string value, ParseData* reqDataP)
{
  reqDataP->qcr.res.restrictions += 1;
  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  reqDataP->qcr.res.restriction.attributeExpression.set(value);

  if (value == "")
  {
    reqDataP->errorString = "Empty attribute expression";
    LM_W(("Empty attribute expression"));
  }

  return "OK";
}



/* ****************************************************************************
*
* operationScope - 
*/
static std::string operationScope(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an operationScope"));
  
  reqDataP->qcr.scopeP = new Scope();
  reqDataP->qcr.res.restriction.scopeVector.push_back(reqDataP->qcr.scopeP);
  reqDataP->qcr.scopeP->type  = "not in use";
  reqDataP->qcr.scopeP->value = "not in use";

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->qcr.scopeP->type = value;
   LM_T(LmtParse, ("Set scope 'type' to '%s' for a scope", reqDataP->qcr.scopeP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(std::string path, std::string value, ParseData* reqDataP)
{
  if (reqDataP->qcr.scopeP->type == "FIWARE_Location")
  {
    reqDataP->qcr.scopeP->value = "FIWARE_Location";
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqDataP->qcr.scopeP->type.c_str()));
  }
  else
  {
    reqDataP->qcr.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), reqDataP->qcr.scopeP->type.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* circle - 
*/
static std::string circle(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circle"));
  reqDataP->qcr.scopeP->scopeType = ScopeAreaCircle;
  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static std::string circleCenterLatitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.origin.latitude = atof(value.c_str());

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static std::string circleCenterLongitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.origin.longitude = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* circleRadius - 
*/
static std::string circleRadius(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  reqDataP->qcr.scopeP->circle.radius = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* qcrParseVector -
*/
JsonNode jsonQcrParseVector[] =
{
   { "/entities",                   jsonNullTreat      },
   { "/entities/entity",            entityId           },
   { "/entities/entity/id",         entityIdId         },
   { "/entities/entity/type",       entityIdType       },
   { "/entities/entity/isPattern",  entityIdIsPattern  },

   { "/attributes",                 attributeList       },
   { "/attributes/attribute",       attribute           },

   { "/restriction",                     restriction            },
   { "/restriction/attributeExpression", attributeExpression    },
   { "/restriction/scopes",              jsonNullTreat          },
   { "/restriction/scopes/scope",        operationScope         },
   { "/restriction/scopes/scope/type",   scopeType              },
   { "/restriction/scopes/scope/value",  scopeValue             },

  { "/restriction/scopes/scope/value/circle",                  circle                     },
  { "/restriction/scopes/scope/value/circle/center_latitude",  circleCenterLatitude       },
  { "/restriction/scopes/scope/value/circle/center_longitude", circleCenterLongitude      },
  { "/restriction/scopes/scope/value/circle/radius",           circleRadius               },

   { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonQcrInit -
*/
void jsonQcrInit(ParseData* reqDataP)
{
  jsonQcrRelease(reqDataP);

  reqDataP->qcr.entityIdP              = NULL;
  reqDataP->qcr.scopeP                 = NULL;
  reqDataP->errorString                = "";

  reqDataP->qcr.res.restrictions           = 0;
  reqDataP->qcr.res.restriction.attributeExpression.set("");
}



/* ****************************************************************************
*
* jsonQcrRelease -
*/
void jsonQcrRelease(ParseData* reqDataP)
{
  reqDataP->qcr.res.release();
}



/* ****************************************************************************
*
* jsonQcrCheck -
*/
std::string jsonQcrCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
   return reqDataP->qcr.res.check(DiscoverContextAvailability, ciP->outFormat, "", reqDataP->errorString, reqDataP->qcr.res.restrictions);
}



#define PRINTF printf
/* ****************************************************************************
*
* jsonQcrPresent -
*/
void jsonQcrPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  PRINTF("\n\n");
  reqDataP->qcr.res.present("");
}
