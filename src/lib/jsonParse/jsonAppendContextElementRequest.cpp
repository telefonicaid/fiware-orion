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
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/Request.h"
#include "jsonParse/jsonParse.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonAppendContextElementRequest.h"
#include "jsonParse/jsonNullTreat.h"


/* ****************************************************************************
*
* attributeDomainName - 
*/
static std::string attributeDomainName(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attributeDomainName"));
  reqData->acer.res.attributeDomainName.set(value);
  return "OK";
}



/* ****************************************************************************
*
* contextAttribute -
*/
static std::string contextAttribute(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqData->acer.attributeP = new ContextAttribute();
  reqData->acer.res.contextAttributeVector.push_back(reqData->acer.attributeP);
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeName -
*/
static std::string contextAttributeName(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute name: %s", value.c_str()));
  reqData->acer.attributeP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeType -
*/
static std::string contextAttributeType(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute type: %s", value.c_str()));
  reqData->acer.attributeP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* contextAttributeValue -
*/
static std::string contextAttributeValue(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute value: %s", value.c_str()));
  reqData->acer.attributeP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadata - 
*/
static std::string contextMetadata(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));
  reqData->acer.metadataP = new Metadata();
  reqData->acer.attributeP->metadataVector.push_back(reqData->acer.metadataP);
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataName - 
*/
static std::string contextMetadataName(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", value.c_str()));
  reqData->acer.metadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataType - 
*/
static std::string contextMetadataType(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", value.c_str()));
  reqData->acer.metadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* contextMetadataValue - 
*/
static std::string contextMetadataValue(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", value.c_str()));
  reqData->acer.metadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadata - 
*/
static std::string domainMetadata(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));
  reqData->acer.domainMetadataP = new Metadata();
  reqData->acer.res.domainMetadataVector.push_back(reqData->acer.domainMetadataP);
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataName - 
*/
static std::string domainMetadataName(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", value.c_str()));
  reqData->acer.domainMetadataP->name = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataType - 
*/
static std::string domainMetadataType(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", value.c_str()));
  reqData->acer.domainMetadataP->type = value;
  return "OK";
}



/* ****************************************************************************
*
* domainMetadataValue - 
*/
static std::string domainMetadataValue(std::string path, std::string value, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", value.c_str()));
  reqData->acer.domainMetadataP->value = value;
  return "OK";
}



/* ****************************************************************************
*
* jsonAcerParseVector -
*/
JsonNode jsonAcerParseVector[] =
{
  { "/attributeDomainName",                          attributeDomainName   },

  { "/attributes",                                   jsonNullTreat         },
  { "/attributes/attribute",                         contextAttribute      },
  { "/attributes/attribute/name",                    contextAttributeName  },
  { "/attributes/attribute/type",                    contextAttributeType  },
  { "/attributes/attribute/value",                   contextAttributeValue },

  { "/attributes/attribute/metadatas",                jsonNullTreat         },
  { "/attributes/attribute/metadatas/metadata",       contextMetadata       },
  { "/attributes/attribute/metadatas/metadata/name",  contextMetadataName   },
  { "/attributes/attribute/metadatas/metadata/type",  contextMetadataType   },
  { "/attributes/attribute/metadatas/metadata/value", contextMetadataValue  },

  { "/metadatas",                                    jsonNullTreat         },
  { "/metadatas/metadata",                           domainMetadata        },
  { "/metadatas/metadata/name",                      domainMetadataName    },
  { "/metadatas/metadata/type",                      domainMetadataType    },
  { "/metadatas/metadata/value",                     domainMetadataValue   },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonAcerInit -
*/
void jsonAcerInit(ParseData* reqData)
{
  reqData->acer.res.attributeDomainName.set("");

  reqData->acer.attributeP       = NULL;
  reqData->acer.metadataP        = NULL;
  reqData->acer.domainMetadataP  = NULL;
}



/* ****************************************************************************
*
* jsonAcerRelease -
*/
void jsonAcerRelease(ParseData* reqData)
{
  reqData->acer.res.release();
}



/* ****************************************************************************
*
* jsonAcerCheck -
*/
std::string jsonAcerCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->acer.res.check(AppendContextElement, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* jsonAcerPresent -
*/
void jsonAcerPresent(ParseData* reqData)
{
  reqData->acer.res.present("");
}
