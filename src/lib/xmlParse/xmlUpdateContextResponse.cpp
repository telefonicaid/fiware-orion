/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "orionTypes/areas.h"
#include "ngsi/Request.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Restriction.h"
#include "ngsi10/UpdateContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlUpdateContextResponse.h"
#include "xmlParse/xmlParse.h"

using namespace orion;



/* ****************************************************************************
*
* contextElementResponse -
*/
static int contextElementResponse(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextElementResponse"));
  parseDataP->upcrs.cerP = new ContextElementResponse();
  parseDataP->upcrs.res.contextElementResponseVector.push_back(parseDataP->upcrs.cerP);
  return 0;
}



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  std::string es = entityIdParse(UpdateContext, node, &parseDataP->upcrs.cerP->contextElement.entityId);

  if (es != "OK")
    parseDataP->errorString = es;

  return 0;
}



/* ****************************************************************************
*
* entityIdId -
*/
static int entityIdId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  if (parseDataP->upcrs.cerP != NULL)
  {
    parseDataP->upcrs.cerP->contextElement.entityId.id = node->value();
  }
  else
  {
    alarmMgr.badInput(clientIp, "XML parse error");

    parseDataP->errorString = "Bad Input (XML parse error)";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* contextAttribute -
*/
static int contextAttribute(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute"));
  parseDataP->upcrs.attributeP = new ContextAttribute();
  parseDataP->upcrs.attributeP->valueType = orion::ValueTypeNone;
  parseDataP->upcrs.cerP->contextElement.contextAttributeVector.push_back(parseDataP->upcrs.attributeP);
  return 0;
}



/* ****************************************************************************
*
* contextAttributeName -
*/
static int contextAttributeName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: %s", node->value()));
  parseDataP->upcrs.attributeP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeType -
*/
static int contextAttributeType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: %s", node->value()));
  parseDataP->upcrs.attributeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeValue -
*/
static int contextAttributeValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: %s", node->value()));
  parseDataP->lastContextAttribute = parseDataP->upcrs.attributeP;
  parseDataP->lastContextAttribute->typeFromXmlAttribute = xmlTypeAttributeGet(node);
  parseDataP->upcrs.attributeP->stringValue = node->value();
  parseDataP->upcrs.attributeP->valueType = orion::ValueTypeString;
  return 0;
}



/* ****************************************************************************
*
* contextMetadata -
*/
static int contextMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata"));
  parseDataP->upcrs.metadataP = new Metadata();
  parseDataP->upcrs.attributeP->metadataVector.push_back(parseDataP->upcrs.metadataP);
  return 0;
}



/* ****************************************************************************
*
* contextMetadataName -
*/
static int contextMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  parseDataP->upcrs.metadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataType -
*/
static int contextMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  parseDataP->upcrs.metadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue -
*/
static int contextMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  parseDataP->upcrs.metadataP->stringValue = node->value();
  return 0;
}



/* ****************************************************************************
*
* statusCodeCode -
*/
static int statusCodeCode(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode code: '%s'", node->value()));
  parseDataP->upcrs.cerP->statusCode.code = (HttpStatusCode) atoi(node->value());
  return 0;
}



/* ****************************************************************************
*
* statusCodeReasonPhrase -
*/
static int statusCodeReasonPhrase(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode reasonPhrase: '%s'", node->value()));
  parseDataP->upcrs.cerP->statusCode.reasonPhrase = node->value();  // OK - parsing step
  return 0;
}



/* ****************************************************************************
*
* statusCodeDetails -
*/
static int statusCodeDetails(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode details: '%s'", node->value()));
  parseDataP->upcrs.cerP->statusCode.details = node->value();
  return 0;
}



/* ****************************************************************************
*
* errorCodeCode -
*/
static int errorCodeCode(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode code: '%s'", node->value()));
  parseDataP->upcrs.res.errorCode.code = (HttpStatusCode) atoi(node->value());
  return 0;
}



/* ****************************************************************************
*
* errorCodeReasonPhrase -
*/
static int errorCodeReasonPhrase(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode reasonPhrase: '%s'", node->value()));
  parseDataP->upcrs.res.errorCode.reasonPhrase = node->value();  // OK - parsing step
  return 0;
}



/* ****************************************************************************
*
* errorCodeDetails -
*/
static int errorCodeDetails(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an errorCode details: '%s'", node->value()));
  parseDataP->upcrs.res.errorCode.details = node->value();
  return 0;
}



#define UCR  "/updateContextResponse"
#define CR_L "/contextResponseList"
#define CER  "/contextElementResponse"
#define CE   "/contextElement"
#define CA_L "/contextAttributeList"
#define CA   "/contextAttribute"
/* ****************************************************************************
*
* upcrsParseVector -
*/
XmlNode upcrsParseVector[] =
{
  { "/updateContextResponse",                                   nullTreat               },

  { UCR "/contextResponseList",                                 nullTreat               },
  { UCR CR_L "/contextElementResponse",                         contextElementResponse  },
  { UCR CR_L CER "/contextElement",                             nullTreat               },
  { UCR CR_L CER CE "/entityId",                                entityId                },
  { UCR CR_L CER CE "/entityId/id",                             entityIdId              },

  { UCR CR_L CER CE "/contextAttributeList",                    nullTreat               },
  { UCR CR_L CER CE CA_L "/contextAttribute",                   contextAttribute        },
  { UCR CR_L CER CE CA_L CA "/name",                            contextAttributeName    },
  { UCR CR_L CER CE CA_L CA "/type",                            contextAttributeType    },
  { UCR CR_L CER CE CA_L CA "/contextValue",                    contextAttributeValue   },

  { UCR CR_L CER CE CA_L CA "/metadata",                        nullTreat               },
  { UCR CR_L CER CE CA_L CA "/metadata/contextMetadata",        contextMetadata         },
  { UCR CR_L CER CE CA_L CA "/metadata/contextMetadata/name",   contextMetadataName     },
  { UCR CR_L CER CE CA_L CA "/metadata/contextMetadata/type",   contextMetadataType     },
  { UCR CR_L CER CE CA_L CA "/metadata/contextMetadata/value",  contextMetadataValue    },

  { UCR CR_L CER "/statusCode",                                 nullTreat               },
  { UCR CR_L CER "/statusCode/code",                            statusCodeCode          },
  { UCR CR_L CER "/statusCode/reasonPhrase",                    statusCodeReasonPhrase  },
  { UCR CR_L CER "/statusCode/details",                         statusCodeDetails       },

  { UCR "/errorCode",                                           nullTreat               },
  { UCR "/errorCode/code",                                      errorCodeCode           },
  { UCR "/errorCode/reasonPhrase",                              errorCodeReasonPhrase   },
  { UCR "/errorCode/details",                                   errorCodeDetails        },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* upcrsInit -
*/
void upcrsInit(ParseData* parseDataP)
{
  upcrsRelease(parseDataP);

  parseDataP->errorString = "";
}



/* ****************************************************************************
*
* upcrsRelease -
*/
void upcrsRelease(ParseData* parseDataP)
{
  parseDataP->upcrs.res.release();
}



/* ****************************************************************************
*
* upcrsCheck -
*/
std::string upcrsCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->upcrs.res.check(ciP, UpdateContext, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* upcrsPresent -
*/
void upcrsPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  LM_T(LmtPresent, ("\n\n"));
  parseDataP->upcrs.res.present("");
}
