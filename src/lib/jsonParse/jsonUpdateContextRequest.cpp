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
#include "jsonParse/JsonNode.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "parse/nullTreat.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* contextElement - 
*/
static std::string contextElement(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("new contextElement"));
  reqDataP->upcr.eP = new Entity();

  reqDataP->upcr.res.contextElementVector.push_back(reqDataP->upcr.eP);

  reqDataP->upcr.eP->id          = "";
  reqDataP->upcr.eP->type        = "";
  reqDataP->upcr.eP->isPattern   = "false";

  return "OK";
}



/* ****************************************************************************
*
* entityIdId - 
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->upcr.eP->id = value;
  LM_T(LmtParse, ("Set 'id' to '%s' for an entity", reqDataP->upcr.eP->id.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdType - 
*/
static std::string entityIdType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->upcr.eP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for an entity", reqDataP->upcr.eP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* entityIdIsPattern - 
*/
static std::string entityIdIsPattern(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:isPattern: '%s'", value.c_str()));

  reqDataP->upcr.eP->isPattern = value;

  return "OK";
}



/* ****************************************************************************
*
* attribute - 
*/
static std::string attribute(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));

  reqDataP->upcr.attributeP = new ContextAttribute("", "", "");
  reqDataP->upcr.attributeP->valueType = orion::ValueTypeNotGiven;
  reqDataP->upcr.eP->attributeVector.push_back(reqDataP->upcr.attributeP);

  return "OK";
}



/* ****************************************************************************
*
* attributeName - 
*/
static std::string attributeName(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->upcr.attributeP->name = value;
  LM_T(LmtParse, ("Set 'name' to '%s' for a contextElement Attribute", reqDataP->upcr.attributeP->name.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* attributeType - 
*/
static std::string attributeType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->upcr.attributeP->type = value;
  LM_T(LmtParse, ("Set 'type' to '%s' for a contextElement attribute", reqDataP->upcr.attributeP->type.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* attributeValue - 
*/
static std::string attributeValue(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  parseDataP->lastContextAttribute = parseDataP->upcr.attributeP;
  LM_T(LmtCompoundValue, ("Set parseDataP->lastContextAttribute to: %p", parseDataP->lastContextAttribute));

  parseDataP->upcr.attributeP->stringValue = value;
  parseDataP->upcr.attributeP->valueType = orion::ValueTypeString;
  LM_T(LmtParse, ("Set value to '%s' for a contextElement attribute", parseDataP->upcr.attributeP->stringValue.c_str()));

  return "OK";
}



/* ****************************************************************************
*
* metadata - 
*/
static std::string metadata(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Creating a metadata"));

  reqDataP->upcr.contextMetadataP = new Metadata();
  reqDataP->upcr.attributeP->metadataVector.push_back(reqDataP->upcr.contextMetadataP);

  return "OK";
}



/* ****************************************************************************
*
* metadataName - 
*/
static std::string metadataName(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", value.c_str()));
  reqDataP->upcr.contextMetadataP->name = value;

  return "OK";
}



/* ****************************************************************************
*
* metadataType - 
*/
static std::string metadataType(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", value.c_str()));
  reqDataP->upcr.contextMetadataP->type = value;

  return "OK";
}



/* ****************************************************************************
*
* metadataValue - 
*/
static std::string metadataValue(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", value.c_str()));
  reqDataP->upcr.contextMetadataP->stringValue = value;
  reqDataP->upcr.contextMetadataP->valueType = orion::ValueTypeString;
  return "OK";
}



/* ****************************************************************************
*
* updateAction - 
*/
static std::string updateAction(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an updateAction: '%s'", value.c_str()));
  reqDataP->upcr.res.updateActionType = parseActionTypeV1(value);

  return "OK";
}



/* ****************************************************************************
*
* jsonUpcrParseVector -
*/
JsonNode jsonUpcrParseVector[] =
{
  { "/contextElements",                                                               jsonNullTreat       },
  { "/contextElements/contextElement",                                                contextElement      },
  { "/contextElements/contextElement/type",                                           entityIdType        },
  { "/contextElements/contextElement/isPattern",                                      entityIdIsPattern   },
  { "/contextElements/contextElement/id",                                             entityIdId          },  
  { "/contextElements/contextElement/attributes",                                     jsonNullTreat       },
  { "/contextElements/contextElement/attributes/attribute",                           attribute           },
  { "/contextElements/contextElement/attributes/attribute/name",                      attributeName       },
  { "/contextElements/contextElement/attributes/attribute/type",                      attributeType       },
  { "/contextElements/contextElement/attributes/attribute/value",                     attributeValue      },
  { "/contextElements/contextElement/attributes/attribute/metadatas",                 jsonNullTreat       },
  { "/contextElements/contextElement/attributes/attribute/metadatas/metadata",        metadata            },
  { "/contextElements/contextElement/attributes/attribute/metadatas/metadata/name",   metadataName        },
  { "/contextElements/contextElement/attributes/attribute/metadatas/metadata/type",   metadataType        },
  { "/contextElements/contextElement/attributes/attribute/metadatas/metadata/value",  metadataValue       },
  { "/updateAction",                                                                  updateAction        },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* jsonUpcrInit - 
*/
void jsonUpcrInit(ParseData* reqDataP)
{
  reqDataP->upcr.eP                     = NULL;
  reqDataP->upcr.attributeP             = NULL;
  reqDataP->upcr.contextMetadataP       = NULL;
  reqDataP->errorString                 = "";
}



/* ****************************************************************************
*
* jsonUpcrRelease - 
*/
void jsonUpcrRelease(ParseData* reqDataP)
{
  reqDataP->upcr.res.release();
}



/* ****************************************************************************
*
* jsonUpcrCheck - 
*/
std::string jsonUpcrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
  return reqData->upcr.res.check(ciP->apiVersion, asJsonObject, reqData->errorString);
}

