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

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/ContextElement.h"
#include "ngsi/EntityId.h"
#include "ngsi/Metadata.h"
#include "ngsi10/UpdateContextRequest.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUpdateContextRequest.h"



/* ****************************************************************************
*
* contextElement - 
*/
static int contextElement(xml_node<>* node, ParseData* reqData)
{
  reqData->upcr.ceP       = new ContextElement();
  reqData->upcr.entityIdP = &reqData->upcr.ceP->entityId;

  reqData->upcr.entityIdP->id = "";

  reqData->upcr.res.contextElementVector.push_back(reqData->upcr.ceP);

  return 0;
}



/* ****************************************************************************
*
* entityId - 
*/
static int entityId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId"));

  std::string es = entityIdParse(SubscribeContext, node, reqData->upcr.entityIdP);

  if (es != "OK")
  {
    reqData->errorString = es;
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* entityIdId - 
*/
static int entityIdId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  reqData->upcr.entityIdP->id = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextAttribute - 
*/
static int contextAttribute(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Creating an attribute"));
  reqData->upcr.attributeP = new ContextAttribute();
  reqData->upcr.ceP->contextAttributeVector.push_back(reqData->upcr.attributeP);
  return 0;
}



/* ****************************************************************************
*
* contextAttributeName - 
*/
static int contextAttributeName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute name: '%s'", node->value()));
  reqData->upcr.attributeP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeType - 
*/
static int contextAttributeType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an attribute type: '%s'", node->value()));
  reqData->upcr.attributeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeValue - 
*/
static int contextAttributeValue(xml_node<>* node, ParseData* parseDataP)
{
  parseDataP->lastContextAttribute = parseDataP->upcr.attributeP;
  parseDataP->lastContextAttribute->typeFromXmlAttribute = xmlTypeAttributeGet(node);
  LM_T(LmtCompoundValue, ("Set parseDataP->lastContextAttribute (type: '%s') to: %p",
                          parseDataP->lastContextAttribute->typeFromXmlAttribute.c_str(),
                          parseDataP->lastContextAttribute));
  
  LM_T(LmtParse, ("Got an attribute value: '%s'", node->value()));
  parseDataP->upcr.attributeP->value = node->value();

  if (parseDataP->upcr.attributeP->value == " ")
    parseDataP->upcr.attributeP->value = "";

  return 0;
}



/* ****************************************************************************
*
* contextMetadata - 
*/
static int contextMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Creating a metadata"));
  reqData->upcr.contextMetadataP = new Metadata();
  reqData->upcr.attributeP->metadataVector.push_back(reqData->upcr.contextMetadataP);
  return 0;
}



/* ****************************************************************************
*
* contextMetadataName - 
*/
static int contextMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", node->value()));
  reqData->upcr.contextMetadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataType - 
*/
static int contextMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", node->value()));
  reqData->upcr.contextMetadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue - 
*/
static int contextMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", node->value()));
  reqData->upcr.contextMetadataP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadata - 
*/
static int domainMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Creating a metadata"));
  reqData->upcr.domainMetadataP = new Metadata();
  reqData->upcr.ceP->domainMetadataVector.push_back(reqData->upcr.domainMetadataP);
  return 0;
}



/* ****************************************************************************
*
* domainMetadataName - 
*/
static int domainMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", node->value()));
  reqData->upcr.domainMetadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadataType - 
*/
static int domainMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", node->value()));
  reqData->upcr.domainMetadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* domainMetadataValue - 
*/
static int domainMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", node->value()));
  reqData->upcr.domainMetadataP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* updateAction - 
*/
static int updateAction(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an update action: '%s'", node->value()));
  reqData->upcr.res.updateActionType.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* upcrInit - 
*/
void upcrInit(ParseData* reqData)
{
  reqData->upcr.ceP              = NULL;
  reqData->upcr.entityIdP        = NULL;
  reqData->upcr.attributeP       = NULL;
  reqData->upcr.contextMetadataP = NULL;
  reqData->upcr.domainMetadataP  = NULL;
  reqData->errorString           = "";

  reqData->upcr.res.init();
}



/* ****************************************************************************
*
* upcrRelease - 
*/
void upcrRelease(ParseData* reqData)
{
   reqData->upcr.res.release();
}




/* ****************************************************************************
*
* upcrCheck - 
*/
std::string upcrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->upcr.res.check(UpdateContext, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* upcrPresent - 
*/
void upcrPresent(ParseData* reqData)
{
  reqData->upcr.res.present("");
}



/* ****************************************************************************
*
* upcrParseVector - 
*/
XmlNode upcrParseVector[] =
{
  { "/updateContextRequest", nullTreat },
  { "/updateContextRequest/contextElementList", nullTreat },

  { "/updateContextRequest/contextElementList/contextElement", contextElement },

  { "/updateContextRequest/contextElementList/contextElement/entityId",           entityId          },
  { "/updateContextRequest/contextElementList/contextElement/entityId/id",        entityIdId        },
  
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList",                               nullTreat              },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute",              contextAttribute       },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/name",         contextAttributeName   },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/type",         contextAttributeType   },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", contextAttributeValue  },
  
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/metadata",                       nullTreat                 },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata",       contextMetadata           },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/name",  contextMetadataName       },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/type",  contextMetadataType       },
  { "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/value", contextMetadataValue      },
  
  { "/updateContextRequest/contextElementList/contextElement/domainMetadata",                       nullTreat            },
  { "/updateContextRequest/contextElementList/contextElement/domainMetadata/contextMetadata",       domainMetadata       },
  { "/updateContextRequest/contextElementList/contextElement/domainMetadata/contextMetadata/name",  domainMetadataName   },
  { "/updateContextRequest/contextElementList/contextElement/domainMetadata/contextMetadata/type",  domainMetadataType   },
  { "/updateContextRequest/contextElementList/contextElement/domainMetadata/contextMetadata/value", domainMetadataValue  },
  
  { "/updateContextRequest/updateAction", updateAction },

  { "LAST", NULL }
};
