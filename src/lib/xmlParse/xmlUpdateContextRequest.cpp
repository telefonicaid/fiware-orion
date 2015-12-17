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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/ContextElement.h"
#include "ngsi/EntityId.h"
#include "ngsi/Metadata.h"
#include "ngsi10/UpdateContextRequest.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUpdateContextRequest.h"
#include "xmlParse/XmlNode.h"



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
static int entityIdId(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  if (reqDataP->upcr.entityIdP != NULL)
  {
    reqDataP->upcr.entityIdP->id = node->value();
  }
  else
  {
    alarmMgr.badInput(clientIp, "XML parse error");
    reqDataP->errorString = "Bad Input (XML parse error)";
    return 1;
  }

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
  reqData->upcr.attributeP->valueType = orion::ValueTypeNone;
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
  parseDataP->upcr.attributeP->stringValue = node->value();
  parseDataP->upcr.attributeP->valueType   = orion::ValueTypeString;

  if (parseDataP->upcr.attributeP->stringValue == " ")
  {
    parseDataP->upcr.attributeP->stringValue = "";
  }

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
  reqData->upcr.contextMetadataP->stringValue = node->value();
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
  reqData->upcr.domainMetadataP->stringValue = node->value();
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
  return reqData->upcr.res.check(ciP, UpdateContext, "", reqData->errorString, 0);
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
#define UCR   "/updateContextRequest"
#define CEL   "/contextElementList"
#define CE    "/contextElement"
#define CAL   "/contextAttributeList"
#define CA    "/contextAttribute"
#define MDL   "/metadata"
#define MD    "/contextMetadata"
#define DMDL  "/domainMetadata"

XmlNode upcrParseVector[] =
{
  { "/updateContextRequest",                   nullTreat                 },
  { UCR "/contextElementList",                 nullTreat                 },

  { UCR CEL "/contextElement",                 contextElement            },

  { UCR CEL CE "/entityId",                    entityId                  },
  { UCR CEL CE "/entityId/id",                 entityIdId                },

  { UCR CEL CE "/contextAttributeList",        nullTreat                 },
  { UCR CEL CE CAL "/contextAttribute",        contextAttribute          },
  { UCR CEL CE CAL CA "/name",                 contextAttributeName      },
  { UCR CEL CE CAL CA "/type",                 contextAttributeType      },
  { UCR CEL CE CAL CA "/contextValue",         contextAttributeValue     },

  { UCR CEL CE CAL CA "/metadata",             nullTreat                 },
  { UCR CEL CE CAL CA MDL "/contextMetadata",  contextMetadata           },
  { UCR CEL CE CAL CA MDL MD "/name",          contextMetadataName       },
  { UCR CEL CE CAL CA MDL MD "/type",          contextMetadataType       },
  { UCR CEL CE CAL CA MDL MD "/value",         contextMetadataValue      },

  { UCR CEL CE "/domainMetadata",              nullTreat                 },
  { UCR CEL CE DMDL "/contextMetadata",        domainMetadata            },
  { UCR CEL CE DMDL MD "/name",                domainMetadataName        },
  { UCR CEL CE DMDL MD "/type",                domainMetadataType        },
  { UCR CEL CE DMDL MD "/value",               domainMetadataValue       },

  { UCR "/updateAction",                       updateAction              },

  { "LAST", NULL }
};
