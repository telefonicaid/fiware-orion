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

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/Request.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlAppendContextElementRequest.h"



/* ****************************************************************************
*
* attributeDomainName - 
*/
static int attributeDomainName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attributeDomainName"));
  reqData->acer.res.attributeDomainName.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* contextAttribute - 
*/
static int contextAttribute(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute"));
  reqData->acer.attributeP = new ContextAttribute();
  reqData->acer.res.contextAttributeVector.push_back(reqData->acer.attributeP);
  return 0;
}



/* ****************************************************************************
*
* contextAttributeName - 
*/
static int contextAttributeName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute name: %s", node->value()));
  reqData->acer.attributeP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeType - 
*/
static int contextAttributeType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute type: %s", node->value()));
  reqData->acer.attributeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeValue - 
*/
static int contextAttributeValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: %s", node->value()));
  parseDataP->lastContextAttribute = parseDataP->acer.attributeP;
  parseDataP->lastContextAttribute->typeFromXmlAttribute = xmlTypeAttributeGet(node);
  parseDataP->acer.attributeP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadata - 
*/
static int contextMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));
  reqData->acer.metadataP = new Metadata();
  reqData->acer.attributeP->metadataVector.push_back(reqData->acer.metadataP);
  return 0;
}



/* ****************************************************************************
*
* contextMetadataName - 
*/
static int contextMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  reqData->acer.metadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataType - 
*/
static int contextMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  reqData->acer.metadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue - 
*/
static int contextMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  reqData->acer.metadataP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadata - 
*/
static int domainMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));
  reqData->acer.domainMetadataP = new Metadata();
  reqData->acer.res.domainMetadataVector.push_back(reqData->acer.domainMetadataP);
  return 0;
}



/* ****************************************************************************
*
* domainMetadataName - 
*/
static int domainMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  reqData->acer.domainMetadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadataType - 
*/
static int domainMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  reqData->acer.domainMetadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadataValue - 
*/
static int domainMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  reqData->acer.domainMetadataP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* acerParseVector - 
*/
XmlNode acerParseVector[] =
{
  { "/appendContextElementRequest",                          nullTreat             },
  { "/appendContextElementRequest/attributeDomainName",      attributeDomainName   },

  { "/appendContextElementRequest/contextAttributeList",                                   nullTreat              },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute",                  contextAttribute       },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/name",             contextAttributeName   },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/type",             contextAttributeType   },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/contextValue",     contextAttributeValue  },

  { "/appendContextElementRequest/contextAttributeList/contextAttribute/metadata",                        nullTreat             },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/metadata/contextMetadata",        contextMetadata       },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/metadata/contextMetadata/name",   contextMetadataName   },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/metadata/contextMetadata/type",   contextMetadataType   },
  { "/appendContextElementRequest/contextAttributeList/contextAttribute/metadata/contextMetadata/value",  contextMetadataValue  },

  { "/appendContextElementRequest/domainMetadata",                                   nullTreat             },
  { "/appendContextElementRequest/domainMetadata/contextMetadata",                   domainMetadata        },
  { "/appendContextElementRequest/domainMetadata/contextMetadata/name",              domainMetadataName    },
  { "/appendContextElementRequest/domainMetadata/contextMetadata/type",              domainMetadataType    },
  { "/appendContextElementRequest/domainMetadata/contextMetadata/value",             domainMetadataValue   },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* acerInit - 
*/
void acerInit(ParseData* reqData)
{
  reqData->acer.res.attributeDomainName.set("");

  reqData->acer.attributeP       = NULL;
  reqData->acer.metadataP        = NULL;
  reqData->acer.domainMetadataP  = NULL;
}



/* ****************************************************************************
*
* acerRelease - 
*/
void acerRelease(ParseData* reqData)
{
  reqData->acer.res.release();
}



/* ****************************************************************************
*
* acerCheck - 
*/
std::string acerCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->acer.res.check(AppendContextElement, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* acerPresent - 
*/
void acerPresent(ParseData* reqData)
{
  reqData->acer.res.present("");
}
