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
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonNullTreat.h"
#include "jsonParse/jsonDiscoverContextAvailabilityRequest.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->dcar.entityIdP = new EntityId();

  LM_T(LmtNew, ("New entityId at %p", reqDataP->dcar.entityIdP));
  reqDataP->dcar.entityIdP->id        = "not in use";
  reqDataP->dcar.entityIdP->type      = "not in use";
  reqDataP->dcar.entityIdP->isPattern = "false";

  reqDataP->dcar.res.entityIdVector.push_back(reqDataP->dcar.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->dcar.entityIdP->id = value;
   LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->dcar.entityIdP->id.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->dcar.entityIdP->type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->dcar.entityIdP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  reqDataP->dcar.entityIdP->isPattern = value;

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

  reqDataP->dcar.res.attributeList.push_back(value);

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
  reqDataP->dcar.res.restrictions += 1;
  return "OK";
}



/* ****************************************************************************
*
* attributeExpression - 
*/
static std::string attributeExpression(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an attributeExpression: '%s'", value.c_str()));

  reqDataP->dcar.res.restriction.attributeExpression.set(value);

  if (value == "")
     LM_RE("Empty attribute expression", ("Empty attribute expression"));

  return "OK";
}



/* ****************************************************************************
*
* operationScope - 
*/
static std::string operationScope(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an operationScope"));
  
  reqDataP->dcar.scopeP = new Scope();
  reqDataP->dcar.res.restriction.scopeVector.push_back(reqDataP->dcar.scopeP);
  reqDataP->dcar.scopeP->type  = "not in use";
  reqDataP->dcar.scopeP->value = "not in use";

  return "OK";
}



/* ****************************************************************************
*
* scopeType - 
*/
static std::string scopeType(std::string path, std::string value, ParseData* reqDataP)
{
   LM_W(("Setting scope type to '%s'", value.c_str()));
   reqDataP->dcar.scopeP->type = value;
   LM_T(LmtParse, ("Set scope 'type' to '%s' for a scope", reqDataP->dcar.scopeP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* scopeValue - 
*/
static std::string scopeValue(std::string path, std::string value, ParseData* reqDataP)
{
  if (reqDataP->dcar.scopeP->type == "FIWARE_Location")
  {
    reqDataP->dcar.scopeP->value = "FIWARE_Location";
    LM_T(LmtParse, ("Preparing scopeValue for '%s'", reqDataP->dcar.scopeP->type.c_str()));
  }
  else
  {
    reqDataP->dcar.scopeP->value = value;
    LM_T(LmtParse, ("Got a scopeValue: '%s' for scopeType '%s'", value.c_str(), reqDataP->dcar.scopeP->type.c_str()));
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
  reqDataP->dcar.scopeP->scopeType = ScopeAreaCircle;
  return "OK";
}



/* ****************************************************************************
*
* circleCenterLatitude - 
*/
static std::string circleCenterLatitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLatitude: %s", value.c_str()));
  reqDataP->dcar.scopeP->circle.origin.latitude = atof(value.c_str());

  return "OK";
}



/* ****************************************************************************
*
* circleCenterLongitude - 
*/
static std::string circleCenterLongitude(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleCenterLongitude: %s", value.c_str()));
  reqDataP->dcar.scopeP->circle.origin.longitude = atof(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* circleRadius - 
*/
static std::string circleRadius(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a circleRadius: %s", value.c_str()));
  reqDataP->dcar.scopeP->circle.radius = atof(value.c_str());
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
   return reqDataP->dcar.res.check(DiscoverContextAvailability, ciP->outFormat, "", reqDataP->errorString, reqDataP->dcar.res.restrictions);
}



#define PRINTF printf
/* ****************************************************************************
*
* jsonDcarPresent - 
*/
void jsonDcarPresent(ParseData* reqDataP)
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
JsonNode jsonDcarParseVector[] =
{
   { "/entities",                         jsonNullTreat         },
   { "/entities/entity",                  entityId              },
   { "/entities/entity/id",               entityIdId            },
   { "/entities/entity/type",             entityIdType          },
   { "/entities/entity/isPattern",        entityIdIsPattern     },

   { "/attributes",                       attributeList         },
   { "/attributes/attribute",             attribute             },

   { "/restriction",                      restriction           },
   { "/restriction/attributeExpression",  attributeExpression   },
   { "/restriction/scopes",               jsonNullTreat         },
   { "/restriction/scopes/scope",         operationScope        },
   { "/restriction/scopes/scope/type",    scopeType             },
   { "/restriction/scopes/scope/value",   scopeValue            },

   { "/restriction/scopes/scope/value/circle",                   circle                },
   { "/restriction/scopes/scope/value/circle/center_latitude",   circleCenterLatitude  },
   { "/restriction/scopes/scope/value/circle/center_longitude",  circleCenterLongitude },
   { "/restriction/scopes/scope/value/circle/radius",            circleRadius          },

  { "LAST", NULL }
};
