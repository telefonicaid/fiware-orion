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
#include "jsonParse/jsonNotifyContextAvailabilityRequest.h"

#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", value.c_str()));
  parseDataP->ncar.res.subscriptionId.set(value);
  return "OK";
}



/* ****************************************************************************
*
* contextRegistrationResponse - 
*/
static std::string contextRegistrationResponse(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationResponse"));

  parseDataP->ncar.crrP = new ContextRegistrationResponse();
  parseDataP->ncar.res.contextRegistrationResponseVector.push_back(parseDataP->ncar.crrP);

  return "OK";
}



/* ****************************************************************************
*
* entityId - 
*/
static std::string entityId(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  parseDataP->ncar.entityIdP = new EntityId();
  parseDataP->ncar.crrP->contextRegistration.entityIdVector.push_back(parseDataP->ncar.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId.Id: '%s'", value.c_str()));

  parseDataP->ncar.entityIdP->id = value;
  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId.Type: '%s'", value.c_str()));

  parseDataP->ncar.entityIdP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId.IsPattern: '%s'", value.c_str()));

  parseDataP->ncar.entityIdP->isPattern = value;
  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got an attribute"));

  parseDataP->ncar.craP = new ContextRegistrationAttribute();
  parseDataP->ncar.crrP->contextRegistration.contextRegistrationAttributeVector.push_back(parseDataP->ncar.craP);
  return "OK";
}



/* ****************************************************************************
*
* attributeName - 
*/
static std::string attributeName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got an attributeName: '%s'", value.c_str()));

  parseDataP->ncar.craP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeType - 
*/
static std::string attributeType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got an attributeType: '%s'", value.c_str()));

  parseDataP->ncar.craP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeIsDomain - 
*/
static std::string attributeIsDomain(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got an attributeIsDomain: '%s'", value.c_str()));

  parseDataP->ncar.craP->isDomain = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadata - 
*/
static std::string attributeMetadata(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata: '%s'", value.c_str()));

  parseDataP->ncar.attributeMetadataP = new Metadata();
  parseDataP->ncar.craP->metadataVector.push_back(parseDataP->ncar.attributeMetadataP);

  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataName - 
*/
static std::string attributeMetadataName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataName: '%s'", value.c_str()));
  parseDataP->ncar.attributeMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataType - 
*/
static std::string attributeMetadataType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataType: '%s'", value.c_str()));
  parseDataP->ncar.attributeMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataValue - 
*/
static std::string attributeMetadataValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataValue: '%s'", value.c_str()));
  parseDataP->ncar.attributeMetadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* registrationMetadata - 
*/
static std::string registrationMetadata(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadata: '%s'", value.c_str()));
  parseDataP->ncar.regMetadataP = new Metadata();
  parseDataP->ncar.crrP->contextRegistration.registrationMetadataVector.push_back(parseDataP->ncar.regMetadataP);
  
  return "OK";
}



/* ****************************************************************************
*
* registrationMetadataName - 
*/
static std::string registrationMetadataName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataName: '%s'", value.c_str()));
  parseDataP->ncar.regMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* registrationMetadataType - 
*/
static std::string registrationMetadataType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataType: '%s'", value.c_str()));
  parseDataP->ncar.regMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* registrationMetadataValue - 
*/
static std::string registrationMetadataValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataValue: '%s'", value.c_str()));
  parseDataP->ncar.regMetadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* providingApplication - 
*/
static std::string providingApplication(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a providingApplication: '%s'", value.c_str()));

  parseDataP->ncar.crrP->contextRegistration.providingApplication.set(value);
  return "OK";
}



/* ****************************************************************************
*
* errorCodeCode - 
*/
static std::string errorCodeCode(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode.code: '%s'", value.c_str()));

  parseDataP->ncar.res.errorCode.code = atoi(value.c_str());
  return "OK";
}



/* ****************************************************************************
*
* errorCodeReasonPhrase - 
*/
static std::string errorCodeReasonPhrase(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode.reasonPhrase: '%s'", value.c_str()));

  parseDataP->ncar.res.errorCode.reasonPhrase = value;
  return "OK";
}



/* ****************************************************************************
*
* errorCodeDetails - 
*/
static std::string errorCodeDetails(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode.details: '%s'", value.c_str()));

  parseDataP->ncar.res.errorCode.details = value;
  return "OK";
}



/* ****************************************************************************
*
* jsonNcarParseVector -
*/
JsonNode jsonNcarParseVector[] =
{
   { "/subscriptionId",                                                   subscriptionId                     },
   { "/contextRegistrationResponses",                                     jsonNullTreat                      },         
   { "/contextRegistrationResponses/contextRegistrationResponse",         contextRegistrationResponse        },

   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration",                            jsonNullTreat          },
   
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/entities",                   jsonNullTreat          },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/entities/entity",            entityId               },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/entities/entity/id",         entityIdId             },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/entities/entity/type",       entityIdType           },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/entities/entity/isPattern",  entityIdIsPattern      },

   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes",                    jsonNullTreat          },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute",          attribute              },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/name",     attributeName          },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/type",     attributeType          },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/isDomain", attributeIsDomain      },

   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/metadatas",                 jsonNullTreat            },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/metadatas/metadata",        attributeMetadata        },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/metadatas/metadata/name",   attributeMetadataName    },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/metadatas/metadata/type",   attributeMetadataType    },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/attributes/attribute/metadatas/metadata/value",  attributeMetadataValue   },

   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/metadatas/metadata",        registrationMetadata        },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/metadatas/metadata/name",   registrationMetadataName    },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/metadatas/metadata/type",   registrationMetadataType    },
   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/metadatas/metadata/value",  registrationMetadataValue   },

   { "/contextRegistrationResponses/contextRegistrationResponse/contextRegistration/providingApplication",      providingApplication        },

   { "/errorCode/code",                  errorCodeCode         },
   { "/errorCode/reasonPhrase",          errorCodeReasonPhrase },
   { "/errorCode/details",               errorCodeDetails      },

   { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonNcarInit -
*/
void jsonNcarInit(ParseData* parseDataP)
{
  jsonNcarRelease(parseDataP);

  parseDataP->ncar.crrP                 = NULL;
  parseDataP->ncar.entityIdP            = NULL;
  parseDataP->ncar.craP                 = NULL;
  parseDataP->ncar.attributeMetadataP   = NULL;
  parseDataP->ncar.regMetadataP         = NULL;
}



/* ****************************************************************************
*
* jsonNcarRelease -
*/
void jsonNcarRelease(ParseData* parseDataP)
{
  parseDataP->ncar.res.release();
}



/* ****************************************************************************
*
* jsonNcarCheck -
*/
std::string jsonNcarCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->ncar.res.check(NotifyContext, ciP->outFormat, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* jsonNcarPresent -
*/
void jsonNcarPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  PRINTF("\n\n");
  parseDataP->ncar.res.present("");
}
