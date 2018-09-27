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
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "jsonParse/JsonNode.h"
#include "parse/nullTreat.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* contextRegistration - 
*/
static std::string contextRegistration(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  // LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));
  reqDataP->rcr.crP = new ContextRegistration();
  // LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->rcr.res.contextRegistrationVector.push_back(reqDataP->rcr.crP);
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->rcr.entityIdP = new EntityId();

  reqDataP->rcr.entityIdP->id        = "";
  reqDataP->rcr.entityIdP->type      = "";
  reqDataP->rcr.entityIdP->isPattern = "false";

  reqDataP->rcr.crP->entityIdVector.push_back(reqDataP->rcr.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->rcr.entityIdP->id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->rcr.entityIdP->id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->rcr.entityIdP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->rcr.entityIdP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  if (!isTrue(value) && !isFalse(value))
  {
    return "invalid isPattern value for entity: /" + value + "/";
  }

  reqDataP->rcr.entityIdP->isPattern = value;

  if (isTrue(reqDataP->rcr.entityIdP->isPattern))
  {
    return "isPattern set to true for registrations is currently not supported";
  }

  return "OK";
}



/* ****************************************************************************
*
* crAttribute - 
*/
static std::string crAttribute(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->rcr.attributeP = new ContextRegistrationAttribute("", "");
  reqDataP->rcr.crP->contextRegistrationAttributeVector.push_back(reqDataP->rcr.attributeP);

  return "OK";
}



/* ****************************************************************************
*
* craName - 
*/
static std::string craName(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->rcr.attributeP->name = value;
  LM_T(LmtParse, ("Set 'name' to '%s' for a contextRegistrationAttribute", reqDataP->rcr.attributeP->name.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* craType - 
*/
static std::string craType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->rcr.attributeP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for a contextRegistrationAttribute", reqDataP->rcr.attributeP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* providingApplication - 
*/
static std::string providingApplication(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a providing application: '%s'", value.c_str()));
  reqDataP->rcr.crP->providingApplication.set(value);

  return "OK";
}



/* ****************************************************************************
*
* duration - 
*/
static std::string duration(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a duration: '%s'. Saving in %p", value.c_str(), &reqDataP->rcr.res.duration));
  reqDataP->rcr.res.duration.set(value);
  LM_T(LmtParse, ("Got a duration: SAVED!"));
  return "OK";
}



/* ****************************************************************************
*
* registrationId - 
*/
static std::string registrationId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a registration id: '%s'", value.c_str()));
  reqDataP->rcr.res.registrationId.set(value);

  return "OK";
}



#define CR "/contextRegistrations"
/* ****************************************************************************
*
* jsonRcrParseVector -
*/
JsonNode jsonRcrParseVector[] =
{
  { "/contextRegistrations",                                                            jsonNullTreat            },
  { CR "/contextRegistration",                                                          contextRegistration      },
  { CR "/contextRegistration/entities",                                                 jsonNullTreat            },
  { CR "/contextRegistration/entities/entity",                                          entityId                 },
  { CR "/contextRegistration/entities/entity/id",                                       entityIdId               },
  { CR "/contextRegistration/entities/entity/type",                                     entityIdType             },
  { CR "/contextRegistration/entities/entity/isPattern",                                entityIdIsPattern        },

  { CR "/contextRegistration/attributes",                                               jsonNullTreat            },
  { CR "/contextRegistration/attributes/attribute",                                     crAttribute              },
  { CR "/contextRegistration/attributes/attribute/name",                                craName                  },
  { CR "/contextRegistration/attributes/attribute/type",                                craType                  },

  { CR "/contextRegistration/providingApplication",                                     providingApplication     },
  { "/duration",                                                                        duration                 },
  { "/registrationId",                                                                  registrationId           },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonRcrInit - 
*/
void jsonRcrInit(ParseData* reqDataP)
{
  reqDataP->rcr.crP                    = NULL;
  reqDataP->rcr.entityIdP              = NULL;
  reqDataP->rcr.attributeP             = NULL;
  reqDataP->rcr.attributeMetadataP     = NULL;
  reqDataP->rcr.registrationMetadataP  = NULL;
  reqDataP->errorString                = "";
}



/* ****************************************************************************
*
* jsonRcrRelease - 
*/
void jsonRcrRelease(ParseData* reqDataP)
{
  reqDataP->rcr.res.release();
}



/* ****************************************************************************
*
* jsonRcrCheck - 
*/
std::string jsonRcrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->rcr.res.check(ciP->apiVersion, reqData->errorString, 0);
}
