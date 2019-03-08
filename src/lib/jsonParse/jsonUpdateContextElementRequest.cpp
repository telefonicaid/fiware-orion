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
#include "jsonParse/jsonParse.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonUpdateContextElementRequest.h"
#include "parse/nullTreat.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* contextAttribute - 
*/
static std::string contextAttribute(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqData->ucer.attributeP = new ContextAttribute();
  reqData->ucer.res.contextAttributeVector.push_back(reqData->ucer.attributeP);
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeName - 
*/
static std::string contextAttributeName(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute name: %s", value.c_str()));
  reqData->ucer.attributeP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeType - 
*/
static std::string contextAttributeType(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute type: %s", value.c_str()));
  reqData->ucer.attributeP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeValue - 
*/
static std::string contextAttributeValue(const std::string& path, const std::string& value, ParseData* reqData)
{
  reqData->lastContextAttribute = reqData->ucer.attributeP;
  LM_T(LmtParse, ("Got an attribute value: %s", value.c_str()));
  reqData->ucer.attributeP->stringValue = value;
  reqData->ucer.attributeP->valueType = orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadata - 
*/
static std::string contextMetadata(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));
  reqData->ucer.metadataP = new Metadata();
  reqData->ucer.attributeP->metadataVector.push_back(reqData->ucer.metadataP);
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataName - 
*/
static std::string contextMetadataName(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", value.c_str()));
  reqData->ucer.metadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataType - 
*/
static std::string contextMetadataType(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", value.c_str()));
  reqData->ucer.metadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataValue - 
*/
static std::string contextMetadataValue(const std::string& path, const std::string& value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", value.c_str()));
  reqData->ucer.metadataP->stringValue = value;
  reqData->ucer.metadataP->valueType = orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* ucerParseVector - 
*/
JsonNode jsonUcerParseVector[] =
{
  { "/attributes",                                     jsonNullTreat         },
  { "/attributes/attribute",                           contextAttribute      },
  { "/attributes/attribute/name",                      contextAttributeName  },
  { "/attributes/attribute/type",                      contextAttributeType  },
  { "/attributes/attribute/value",                     contextAttributeValue },

  { "/attributes/attribute/metadatas",                 jsonNullTreat         },
  { "/attributes/attribute/metadatas/metadata",        contextMetadata       },
  { "/attributes/attribute/metadatas/metadata/name",   contextMetadataName   },
  { "/attributes/attribute/metadatas/metadata/type",   contextMetadataType   },
  { "/attributes/attribute/metadatas/metadata/value",  contextMetadataValue  },

  { "LAST", NULL }
};

/* ****************************************************************************
*
* ucerInit - 
*/
void jsonUcerInit(ParseData* reqData)
{
  reqData->ucer.attributeP = NULL;
  reqData->ucer.metadataP  = NULL;
}



/* ****************************************************************************
*
* ucerRelease - 
*/
void jsonUcerRelease(ParseData* reqData)
{
  reqData->ucer.res.release();
}



/* ****************************************************************************
*
* ucerCheck - 
*/
std::string jsonUcerCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
  return reqData->ucer.res.check(ciP->apiVersion, asJsonObject, UpdateContextElement, reqData->errorString);
}
