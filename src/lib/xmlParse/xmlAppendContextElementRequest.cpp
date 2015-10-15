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
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/Request.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlAppendContextElementRequest.h"
#include "xmlParse/xmlParse.h"



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
  reqData->acer.attributeP->valueType = orion::ValueTypeNone;
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
  parseDataP->acer.attributeP->stringValue = node->value();
  parseDataP->acer.attributeP->valueType = orion::ValueTypeString;
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
  reqData->acer.metadataP->stringValue = node->value();
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
  reqData->acer.domainMetadataP->stringValue = node->value();
  return 0;
}



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  std::string es = entityIdParse(RegisterContext, node, &parseDataP->acer.res.entity);

  LM_T(LmtParse, ("Got an entityId"));

  if (es != "OK")
  {
    parseDataP->errorString = es;
  }

  return 0;
}



/* ****************************************************************************
*
* entityIdId -
*/
static int entityIdId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entity:id: '%s'", node->value()));
  parseDataP->acer.res.entity.id = node->value();

  return 0;
}



#define CATTR "/appendContextElementRequest/contextAttributeList/contextAttribute"
#define DM    "/appendContextElementRequest/domainMetadata"
/* ****************************************************************************
*
* acerParseVector - 
*/
XmlNode acerParseVector[] =
{
  { "/appendContextElementRequest",                          nullTreat             },
  { "/appendContextElementRequest/attributeDomainName",      attributeDomainName   },

  { "/appendContextElementRequest/entityId",                 entityId              },
  { "/appendContextElementRequest/entityId/id",              entityIdId            },

  { "/appendContextElementRequest/contextAttributeList",     nullTreat             },
  { CATTR,                                                   contextAttribute      },
  { CATTR "/name",                                           contextAttributeName  },
  { CATTR "/type",                                           contextAttributeType  },
  { CATTR "/contextValue",                                   contextAttributeValue },

  { CATTR "/metadata",                                       nullTreat             },
  { CATTR "/metadata/contextMetadata",                       contextMetadata       },
  { CATTR "/metadata/contextMetadata/name",                  contextMetadataName   },
  { CATTR "/metadata/contextMetadata/type",                  contextMetadataType   },
  { CATTR "/metadata/contextMetadata/value",                 contextMetadataValue  },

  { DM,                                                      nullTreat             },
  { DM "/contextMetadata",                                   domainMetadata        },
  { DM "/contextMetadata/name",                              domainMetadataName    },
  { DM "/contextMetadata/type",                              domainMetadataType    },
  { DM "/contextMetadata/value",                             domainMetadataValue   },

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
  return reqData->acer.res.check(ciP, AppendContextElement, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* acerPresent - 
*/
void acerPresent(ParseData* reqData)
{
  reqData->acer.res.present("");
}
