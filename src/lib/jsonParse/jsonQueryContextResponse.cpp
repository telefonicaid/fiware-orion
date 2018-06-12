/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "orionTypes/areas.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonQueryContextResponse.h"
#include "parse/nullTreat.h"

#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"

using namespace orion;



/* ****************************************************************************
*
* contextResponse - 
*/
static std::string contextResponse(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  parseDataP->qcrs.cerP = new ContextElementResponse();
  parseDataP->qcrs.res.contextElementResponseVector.push_back(parseDataP->qcrs.cerP);
  return "OK";
}


/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  parseDataP->qcrs.cerP->contextElement.entityId.id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", parseDataP->qcrs.cerP->contextElement.entityId.id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  parseDataP->qcrs.cerP->contextElement.entityId.type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for an entity", parseDataP->qcrs.cerP->contextElement.entityId.type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));
  parseDataP->qcrs.cerP->contextElement.entityId.isPattern = value;

  if (!isTrue(value) && !isFalse(value))
  {
    return "invalid isPattern value for entity: /" + value + "/";
  }

  return "OK";
}



/* ****************************************************************************
*
* attributeDomainName - 
*/
static std::string attributeDomainName(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeDomainName: '%s'", value.c_str()));
  parseDataP->qcrs.cerP->contextElement.attributeDomainName.set(value);
  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attribute"));
  parseDataP->qcrs.attributeP = new ContextAttribute();
  parseDataP->qcrs.cerP->contextElement.contextAttributeVector.push_back(parseDataP->qcrs.attributeP);
  return "OK";
}



/* ****************************************************************************
*
* attributeName - 
*/
static std::string attributeName(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: '%s'", value.c_str()));
  parseDataP->qcrs.attributeP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeType - 
*/
static std::string attributeType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: '%s'", value.c_str()));
  parseDataP->qcrs.attributeP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeValue - 
*/
static std::string attributeValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: '%s'", value.c_str()));
  parseDataP->lastContextAttribute = parseDataP->qcrs.attributeP;
  parseDataP->qcrs.attributeP->stringValue = value;
  parseDataP->qcrs.attributeP->valueType = orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadata - 
*/
static std::string attributeMetadata(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attributeMetadata"));
  parseDataP->qcrs.metadataP = new Metadata();
  parseDataP->qcrs.attributeP->metadataVector.push_back(parseDataP->qcrs.metadataP);
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataName - 
*/
static std::string attributeMetadataName(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata name: '%s'", value.c_str()));
  parseDataP->qcrs.metadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataType - 
*/
static std::string attributeMetadataType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata type: '%s'", value.c_str()));
  parseDataP->qcrs.metadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* attributeMetadataValue - 
*/
static std::string attributeMetadataValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata value: '%s'", value.c_str()));
  parseDataP->qcrs.metadataP->stringValue = value;
  parseDataP->qcrs.metadataP->valueType = orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadata - 
*/
static std::string domainMetadata(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating a domainMetadata"));
  parseDataP->qcrs.domainMetadataP = new Metadata();
  parseDataP->qcrs.cerP->contextElement.domainMetadataVector.push_back(parseDataP->qcrs.domainMetadataP);
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataName - 
*/
static std::string domainMetadataName(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata name: '%s'", value.c_str()));
  parseDataP->qcrs.domainMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataType - 
*/
static std::string domainMetadataType(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata type: '%s'", value.c_str()));
  parseDataP->qcrs.domainMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataValue - 
*/
static std::string domainMetadataValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a domainMetadata value: '%s'", value.c_str()));
  parseDataP->qcrs.domainMetadataP->stringValue = value;
  parseDataP->qcrs.domainMetadataP->valueType =orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* statusCodeCode - 
*/
static std::string statusCodeCode(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode code: '%s'", value.c_str()));
  parseDataP->qcrs.cerP->statusCode.code = (HttpStatusCode) atoi(value.c_str());
  LM_T(LmtParse, ("Got a statusCode code: %d", (int) parseDataP->qcrs.cerP->statusCode.code));
  return "OK";
}



/* ****************************************************************************
*
* statusCodeReasonPhrase - 
*/
static std::string statusCodeReasonPhrase(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode reasonPhrase: '%s'", value.c_str()));
  parseDataP->qcrs.cerP->statusCode.reasonPhrase = value;  // OK - parsing step reading reasonPhrase
  return "OK";
}



/* ****************************************************************************
*
* statusCodeDetails - 
*/
static std::string statusCodeDetails(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode details: '%s'", value.c_str()));
  parseDataP->qcrs.cerP->statusCode.details = value;
  return "OK";
}



/* ****************************************************************************
*
* errorCodeCode - 
*/
static std::string errorCodeCode(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode code: '%s'", value.c_str()));
  parseDataP->qcrs.res.errorCode.code = (HttpStatusCode) atoi(value.c_str());
  LM_T(LmtParse, ("Got a errorCode code: %d", (int) parseDataP->qcrs.res.errorCode.code));
  return "OK";
}



/* ****************************************************************************
*
* errorCodeReasonPhrase - 
*/
static std::string errorCodeReasonPhrase(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode reasonPhrase: '%s'", value.c_str()));
  parseDataP->qcrs.res.errorCode.reasonPhrase = value;  // OK - parsing step reading reasonPhrase
  return "OK";
}



/* ****************************************************************************
*
* errorCodeDetails - 
*/
static std::string errorCodeDetails(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode details: '%s'", value.c_str()));
  parseDataP->qcrs.res.errorCode.details = value;
  return "OK";
}




#define CELEM "/contextResponses/contextResponse/contextElement"
/* ****************************************************************************
*
* qcrsParseVector -
*/
JsonNode jsonQcrsParseVector[] =
{
  { "/contextResponses",                                          jsonNullTreat            },
  { "/contextResponses/contextResponse",                          contextResponse          },

  { CELEM,                                                        jsonNullTreat            },
  { CELEM "/id",                                                  entityIdId               },
  { CELEM "/type",                                                entityIdType             },
  { CELEM "/isPattern",                                           entityIdIsPattern        },

  { CELEM "/attributeDomainName",                                 attributeDomainName      },

  { CELEM "/attributes",                                          jsonNullTreat            },
  { CELEM "/attributes/attribute",                                attribute                },
  { CELEM "/attributes/attribute/name",                           attributeName            },
  { CELEM "/attributes/attribute/type",                           attributeType            },
  { CELEM "/attributes/attribute/value",                          attributeValue           },

  { CELEM "/attributes/attribute/metadatas",                      jsonNullTreat            },
  { CELEM "/attributes/attribute/metadatas/metadata",             attributeMetadata        },
  { CELEM "/attributes/attribute/metadatas/metadata/name",        attributeMetadataName    },
  { CELEM "/attributes/attribute/metadatas/metadata/type",        attributeMetadataType    },
  { CELEM "/attributes/attribute/metadatas/metadata/value",       attributeMetadataValue   },

  { CELEM "/metadatas",                                           jsonNullTreat            },
  { CELEM "/metadatas/metadata",                                  domainMetadata           },
  { CELEM "/metadatas/metadata/name",                             domainMetadataName       },
  { CELEM "/metadatas/metadata/type",                             domainMetadataType       },
  { CELEM "/metadatas/metadata/value",                            domainMetadataValue      },

  { "/contextResponses/contextResponse/statusCode",               jsonNullTreat            },
  { "/contextResponses/contextResponse/statusCode/code",          statusCodeCode           },
  { "/contextResponses/contextResponse/statusCode/reasonPhrase",  statusCodeReasonPhrase   },
  { "/contextResponses/contextResponse/statusCode/details",       statusCodeDetails        },

  { "/errorCode",                                                 jsonNullTreat            },
  { "/errorCode/code",                                            errorCodeCode            },
  { "/errorCode/reasonPhrase",                                    errorCodeReasonPhrase    },
  { "/errorCode/details",                                         errorCodeDetails         },
  
  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonQcrsInit -
*/
void jsonQcrsInit(ParseData* reqDataP)
{
  jsonQcrsRelease(reqDataP);

  reqDataP->qcrs.cerP                  = NULL;
  reqDataP->qcrs.attributeP            = NULL;
  reqDataP->qcrs.metadataP             = NULL;
  reqDataP->qcrs.domainMetadataP       = NULL;

  reqDataP->errorString                = "";
}



/* ****************************************************************************
*
* jsonQcrsRelease -
*/
void jsonQcrsRelease(ParseData* reqDataP)
{
  reqDataP->qcrs.res.release();
}



/* ****************************************************************************
*
* jsonQcrsCheck -
*/
std::string jsonQcrsCheck(ParseData* reqDataP, ConnectionInfo* ciP)
{
  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
  return reqDataP->qcrs.res.check(ciP->apiVersion,asJsonObject, reqDataP->errorString);
}
