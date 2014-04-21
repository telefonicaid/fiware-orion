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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonNullTreat.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* contextRegistration - 
*/
static std::string contextRegistration(std::string path, std::string value, ParseData* reqDataP)
{
  //LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));
  reqDataP->rcr.crP = new ContextRegistration();
  //LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));
  
  reqDataP->rcr.res.contextRegistrationVector.push_back(reqDataP->rcr.crP);
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* reqDataP)
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
static std::string entityIdId(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->rcr.entityIdP->id = value;
   LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->rcr.entityIdP->id.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* reqDataP)
{
   reqDataP->rcr.entityIdP->type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->rcr.entityIdP->type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  if (!isTrue(value) && !isFalse(value))
    return "invalid isPattern (boolean) value for entity: '" + value + "'";

  if (isTrue(value))
    return "isPattern set to true for a registration";

  reqDataP->rcr.entityIdP->isPattern = value;

  return "OK";
}



/* ****************************************************************************
*
* crAttribute - 
*/
static std::string crAttribute(std::string path, std::string value, ParseData* reqDataP)
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
static std::string craName(std::string path, std::string value, ParseData* reqDataP)
{
  reqDataP->rcr.attributeP->name = value;
  LM_T(LmtParse, ("Set 'name' to '%s' for a contextRegistrationAttribute", reqDataP->rcr.attributeP->name.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* craType - 
*/
static std::string craType(std::string path, std::string value, ParseData* reqDataP)
{
  reqDataP->rcr.attributeP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for a contextRegistrationAttribute", reqDataP->rcr.attributeP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* craIsDomain - 
*/
static std::string craIsDomain(std::string path, std::string value, ParseData* reqDataP)
{
  if (!isTrue(value) && !isFalse(value))
    return "invalid isDomain (boolean) value for context registration attribute: '" + value + "'";

  reqDataP->rcr.attributeP->isDomain = value;
  LM_T(LmtParse, ("Set 'isDomain' to '%s' for a contextRegistrationAttribute", reqDataP->rcr.attributeP->isDomain.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* craMetadata - 
*/
static std::string craMetadata(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Creating a metadata"));

  reqDataP->rcr.attributeMetadataP = new Metadata();

  reqDataP->rcr.attributeMetadataP->type  = "";
  reqDataP->rcr.attributeMetadataP->name  = "";
  reqDataP->rcr.attributeMetadataP->value = "";

  reqDataP->rcr.attributeP->metadataVector.push_back(reqDataP->rcr.attributeMetadataP);

  return "OK";
}



/* ****************************************************************************
*
* craMetadataName - 
*/
static std::string craMetadataName(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", value.c_str()));
  reqDataP->rcr.attributeMetadataP->name = value;

  return "OK";
}



/* ****************************************************************************
*
* craMetadataType - 
*/
static std::string craMetadataType(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", value.c_str()));
  reqDataP->rcr.attributeMetadataP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* craMetadataValue - 
*/
static std::string craMetadataValue(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", value.c_str()));
  reqDataP->rcr.attributeMetadataP->value = value;

  return "OK";
}



/* ****************************************************************************
*
* regMetadata - 
*/
static std::string regMetadata(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Creating a reg metadata"));

  reqDataP->rcr.registrationMetadataP = new Metadata();
  reqDataP->rcr.registrationMetadataP->type  = "";
  reqDataP->rcr.registrationMetadataP->name  = "";
  reqDataP->rcr.registrationMetadataP->value = "";
  reqDataP->rcr.crP->registrationMetadataVector.push_back(reqDataP->rcr.registrationMetadataP);

  return "OK";
}



/* ****************************************************************************
*
* regMetadataName - 
*/
static std::string regMetadataName(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a reg metadata name: '%s'", value.c_str()));
  reqDataP->rcr.registrationMetadataP->name = value;

  return "OK";
}



/* ****************************************************************************
*
* regMetadataType - 
*/
static std::string regMetadataType(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a reg metadata type: '%s'", value.c_str()));
  reqDataP->rcr.registrationMetadataP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* regMetadataValue - 
*/
static std::string regMetadataValue(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a reg metadata value: '%s'", value.c_str()));
  reqDataP->rcr.registrationMetadataP->value = value;

  return "OK";
}



/* ****************************************************************************
*
* providingApplication - 
*/
static std::string providingApplication(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a providing application: '%s'", value.c_str()));
  reqDataP->rcr.crP->providingApplication.set(value);

  return "OK";
}



/* ****************************************************************************
*
* duration - 
*/
static std::string duration(std::string path, std::string value, ParseData* reqDataP)
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
static std::string registrationId(std::string path, std::string value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a registration id: '%s'", value.c_str()));
  reqDataP->rcr.res.registrationId.set(value);

  return "OK";
}



/* ****************************************************************************
*
* jsonRcrParseVector -
*/
JsonNode jsonRcrParseVector[] =
{
  { "/contextRegistrations",                                                                       jsonNullTreat     },
  { "/contextRegistrations/contextRegistration",                                                   contextRegistration },
  { "/contextRegistrations/contextRegistration/entities",                                          jsonNullTreat     },
  { "/contextRegistrations/contextRegistration/entities/entity",                                   entityId          },
  { "/contextRegistrations/contextRegistration/entities/entity/id",                                entityIdId        },
  { "/contextRegistrations/contextRegistration/entities/entity/type",                              entityIdType      },
  { "/contextRegistrations/contextRegistration/entities/entity/isPattern",                         entityIdIsPattern },

  { "/contextRegistrations/contextRegistration/attributes",                                        jsonNullTreat     },
  { "/contextRegistrations/contextRegistration/attributes/attribute",                              crAttribute       },
  { "/contextRegistrations/contextRegistration/attributes/attribute/name",                         craName           },
  { "/contextRegistrations/contextRegistration/attributes/attribute/type",                         craType           },
  { "/contextRegistrations/contextRegistration/attributes/attribute/isDomain",                     craIsDomain       },

  { "/contextRegistrations/contextRegistration/attributes/attribute/metadatas",                    jsonNullTreat     },
  { "/contextRegistrations/contextRegistration/attributes/attribute/metadatas/metadata",           craMetadata       },
  { "/contextRegistrations/contextRegistration/attributes/attribute/metadatas/metadata/name",      craMetadataName   },
  { "/contextRegistrations/contextRegistration/attributes/attribute/metadatas/metadata/type",      craMetadataType   },
  { "/contextRegistrations/contextRegistration/attributes/attribute/metadatas/metadata/value",     craMetadataValue  },

  { "/contextRegistrations/contextRegistration/metadatas",                                         jsonNullTreat    },
  { "/contextRegistrations/contextRegistration/metadatas/metadata",                                regMetadata      },
  { "/contextRegistrations/contextRegistration/metadatas/metadata/name",                           regMetadataName  },
  { "/contextRegistrations/contextRegistration/metadatas/metadata/type",                           regMetadataType  },
  { "/contextRegistrations/contextRegistration/metadatas/metadata/value",                          regMetadataValue },

  { "/contextRegistrations/contextRegistration/providingApplication",                providingApplication },
  { "/duration",                                                                     duration             },
  { "/registrationId",                                                               registrationId       },

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
  return reqData->rcr.res.check(RegisterContext, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* jsonRcrPresent - 
*/
void jsonRcrPresent(ParseData* reqDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");

  reqDataP->rcr.res.contextRegistrationVector.present("");
  reqDataP->rcr.res.duration.present("");
  reqDataP->rcr.res.registrationId.present("");
}
