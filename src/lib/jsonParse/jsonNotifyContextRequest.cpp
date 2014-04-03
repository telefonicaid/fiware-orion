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
#include "ngsi/Metadata.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonNullTreat.h"
#include "jsonParse/jsonNotifyContextRequest.h"

#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", value.c_str()));
  parseDataP->ncr.res.subscriptionId.set(value);
  return "OK";
}



/* ****************************************************************************
*
* originator - 
*/
static std::string originator(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an originator: '%s'", value.c_str()));
  parseDataP->ncr.res.originator.set(value);
  return "OK";
}



/* ****************************************************************************
*
* contextResponse - 
*/
static std::string contextResponse(std::string path, std::string value, ParseData* parseDataP)
{
  parseDataP->ncr.cerP = new ContextElementResponse();
  parseDataP->ncr.res.contextElementResponseVector.push_back(parseDataP->ncr.cerP);
  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(std::string path, std::string value, ParseData* parseDataP)
{
  parseDataP->ncr.cerP->contextElement.entityId.id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", parseDataP->ncr.cerP->contextElement.entityId.id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(std::string path, std::string value, ParseData* parseDataP)
{
   parseDataP->ncr.cerP->contextElement.entityId.type = value;
   LM_T(LmtParse, ("Set 'type' to '%s' for an entity", parseDataP->ncr.cerP->contextElement.entityId.type.c_str()));

   return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));
  parseDataP->ncr.cerP->contextElement.entityId.isPattern = value;

  if (!isTrue(value) && !isFalse(value))
    return "invalid isPattern (boolean) value for entity: '" + value + "'";


  return "OK";
}



/* ****************************************************************************
*
* attributeDomainName - 
*/
static std::string attributeDomainName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeDomainName: '%s'", value.c_str()));
  parseDataP->ncr.cerP->contextElement.attributeDomainName.set(value);
  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attribute"));
  parseDataP->ncr.attributeP = new ContextAttribute();
  parseDataP->ncr.cerP->contextElement.contextAttributeVector.push_back(parseDataP->ncr.attributeP);
  return "OK";
}



/* ****************************************************************************
*
* attributeName - 
*/
static std::string attributeName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: '%s'", value.c_str()));
  parseDataP->ncr.attributeP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeType - 
*/
static std::string attributeType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: '%s'", value.c_str()));
  parseDataP->ncr.attributeP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeValue - 
*/
static std::string attributeValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: '%s'", value.c_str()));
  parseDataP->ncr.attributeP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* statusCodeCode - 
*/
static std::string statusCodeCode(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode code: '%s'", value.c_str()));
  parseDataP->ncr.cerP->statusCode.code = (HttpStatusCode) atoi(value.c_str());
  LM_T(LmtParse, ("Got a statusCode code: %d", (int) parseDataP->ncr.cerP->statusCode.code));
  return "OK";
}



/* ****************************************************************************
*
* statusCodeReasonPhrase - 
*/
static std::string statusCodeReasonPhrase(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode reasonPhrase: '%s'", value.c_str()));
  parseDataP->ncr.cerP->statusCode.reasonPhrase = value;
  return "OK";
}



/* ****************************************************************************
*
* statusCodeDetails - 
*/
static std::string statusCodeDetails(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode details: '%s'", value.c_str()));
  parseDataP->ncr.cerP->statusCode.details = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadata - 
*/
static std::string attributeMetadata(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attributeMetadata"));
  parseDataP->ncr.attributeMetadataP = new Metadata();
  parseDataP->ncr.attributeP->metadataVector.push_back(parseDataP->ncr.attributeMetadataP);
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataName - 
*/
static std::string attributeMetadataName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata name: '%s'", value.c_str()));
  parseDataP->ncr.attributeMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataType - 
*/
static std::string attributeMetadataType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata type: '%s'", value.c_str()));
  parseDataP->ncr.attributeMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataValue - 
*/
static std::string attributeMetadataValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata value: '%s'", value.c_str()));
  parseDataP->ncr.attributeMetadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadata - 
*/
static std::string domainMetadata(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating a domainMetadata"));
  parseDataP->ncr.domainMetadataP = new Metadata();
  parseDataP->ncr.cerP->contextElement.domainMetadataVector.push_back(parseDataP->ncr.domainMetadataP);
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataName - 
*/
static std::string domainMetadataName(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata name: '%s'", value.c_str()));
  parseDataP->ncr.domainMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataType - 
*/
static std::string domainMetadataType(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata type: '%s'", value.c_str()));
  parseDataP->ncr.domainMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataValue - 
*/
static std::string domainMetadataValue(std::string path, std::string value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata value: '%s'", value.c_str()));
  parseDataP->ncr.domainMetadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* ncrParseVector -
*/
JsonNode jsonNcrParseVector[] =
{
   { "/subscriptionId",                           subscriptionId         },
   { "/originator",                               originator             },

   { "/contextResponses",                                           jsonNullTreat          },
   { "/contextResponses/contextResponse",                           contextResponse        },
   { "/contextResponses/contextResponse/contextElement",            jsonNullTreat          },
   { "/contextResponses/contextResponse/contextElement/id",         entityIdId             },
   { "/contextResponses/contextResponse/contextElement/type",       entityIdType           },
   { "/contextResponses/contextResponse/contextElement/isPattern",  entityIdIsPattern      },

   { "/contextResponses/contextResponse/contextElement/attributeDomainName",  attributeDomainName      },

   { "/contextResponses/contextResponse/contextElement/attributes",                 jsonNullTreat          },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute",       attribute              },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/name",  attributeName          },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/type",  attributeType          },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/value", attributeValue         },

   { "/contextResponses/contextResponse/contextElement/attributes/attribute/metadatas",                 jsonNullTreat            },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/metadatas/metadata",        attributeMetadata        },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/metadatas/metadata/name",   attributeMetadataName    },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/metadatas/metadata/type",   attributeMetadataType    },
   { "/contextResponses/contextResponse/contextElement/attributes/attribute/metadatas/metadata/value",  attributeMetadataValue   },

   { "/contextResponses/contextResponse/contextElement/metadatas",                 jsonNullTreat         },
   { "/contextResponses/contextResponse/contextElement/metadatas/metadata",        domainMetadata        },
   { "/contextResponses/contextResponse/contextElement/metadatas/metadata/name",   domainMetadataName    },
   { "/contextResponses/contextResponse/contextElement/metadatas/metadata/type",   domainMetadataType    },
   { "/contextResponses/contextResponse/contextElement/metadatas/metadata/value",  domainMetadataValue   },


   { "/contextResponses/contextResponse/statusCode",                 jsonNullTreat          },
   { "/contextResponses/contextResponse/statusCode/code",            statusCodeCode         },
   { "/contextResponses/contextResponse/statusCode/reasonPhrase",    statusCodeReasonPhrase },
   { "/contextResponses/contextResponse/statusCode/details",         statusCodeDetails      },

   { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonNcrInit -
*/
void jsonNcrInit(ParseData* parseDataP)
{
  jsonNcrRelease(parseDataP);

  parseDataP->ncr.cerP                 = NULL;
  parseDataP->ncr.attributeP           = NULL;
  parseDataP->ncr.attributeMetadataP   = NULL;
  parseDataP->ncr.domainMetadataP      = NULL;
}



/* ****************************************************************************
*
* jsonNcrRelease -
*/
void jsonNcrRelease(ParseData* parseDataP)
{
  parseDataP->ncr.res.release();
}



/* ****************************************************************************
*
* jsonNcrCheck -
*/
std::string jsonNcrCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->ncr.res.check(NotifyContext, ciP->outFormat, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* jsonNcrPresent -
*/
void jsonNcrPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  PRINTF("\n\n");
  parseDataP->ncr.res.present("");
}
