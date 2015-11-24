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
#include "ngsi/Request.h"
#include "ngsi10/NotifyContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlNotifyContextRequest.h"



/* ****************************************************************************
*
* ncrInit -
*/
void ncrInit(ParseData* parseDataP)
{
  ncrRelease(parseDataP);

  parseDataP->ncr.cerP                 = NULL;
  parseDataP->ncr.attributeP           = NULL;
  parseDataP->ncr.attributeMetadataP   = NULL;
  parseDataP->ncr.domainMetadataP      = NULL;
}



/* ****************************************************************************
*
* ncrRelease -
*/
void ncrRelease(ParseData* parseDataP)
{
  parseDataP->ncr.res.release();
}



/* ****************************************************************************
*
* ncrCheck -
*/
std::string ncrCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->ncr.res.check(ciP, NotifyContext, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* ncrPresent -
*/
void ncrPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtDump))
  {
    return;
  }

  LM_T(LmtDump, ("\n\n"));
  parseDataP->ncr.res.subscriptionId.present("");
}



/* ****************************************************************************
*
* subscriptionId -
*/
static int subscriptionId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", node->value()));
  parseDataP->ncr.res.subscriptionId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* originator -
*/
static int originator(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an originator: '%s'", node->value()));
  parseDataP->ncr.res.originator.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  std::string es = entityIdParse(RegisterContext, node, &parseDataP->ncr.cerP->contextElement.entityId);

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
  LM_T(LmtParse, ("Got an entityId:id: '%s'", node->value()));

  if (parseDataP->ncr.cerP != NULL)
  {
    parseDataP->ncr.cerP->contextElement.entityId.id = node->value();
  }
  else
  {
    LM_W(("Bad Input (XML parse error)"));
    parseDataP->errorString = "Bad Input (XML parse error)";
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* attributeDomainName -
*/
static int attributeDomainName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeDomainName: '%s'", node->value()));
  parseDataP->ncr.cerP->contextElement.attributeDomainName.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* contextElementResponse -
*/
static int contextElementResponse(xml_node<>* node, ParseData* parseDataP)
{
  parseDataP->ncr.cerP = new ContextElementResponse();
  parseDataP->ncr.res.contextElementResponseVector.push_back(parseDataP->ncr.cerP);

  return 0;
}



/* ****************************************************************************
*
* contextAttribute -
*/
static int contextAttribute(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attribute"));

  parseDataP->ncr.attributeP = new ContextAttribute();
  parseDataP->ncr.attributeP->valueType = orion::ValueTypeNone;
  parseDataP->ncr.cerP->contextElement.contextAttributeVector.push_back(parseDataP->ncr.attributeP);

  return 0;
}



/* ****************************************************************************
*
* contextAttributeName -
*/
static int contextAttributeName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: '%s'", node->value()));
  parseDataP->ncr.attributeP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextAttributeType -
*/
static int contextAttributeType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: '%s'", node->value()));
  parseDataP->ncr.attributeP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextAttributeContextValue -
*/
static int contextAttributeContextValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: '%s'", node->value()));
  parseDataP->lastContextAttribute = parseDataP->ncr.attributeP;
  parseDataP->lastContextAttribute->typeFromXmlAttribute = xmlTypeAttributeGet(node);

  parseDataP->ncr.attributeP->stringValue = node->value();
  parseDataP->ncr.attributeP->valueType = orion::ValueTypeString;

  return 0;
}



/* ****************************************************************************
*
* contextMetadata -
*/
static int contextMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));

  reqData->ncr.attributeMetadataP = new Metadata();
  reqData->ncr.attributeP->metadataVector.push_back(reqData->ncr.attributeMetadataP);

  return 0;
}



/* ****************************************************************************
*
* contextMetadataName -
*/
static int contextMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  reqData->ncr.attributeMetadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextMetadataType -
*/
static int contextMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  reqData->ncr.attributeMetadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue -
*/
static int contextMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  reqData->ncr.attributeMetadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* statusCodeCode -
*/
static int statusCodeCode(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode code: '%s'", node->value()));
  parseDataP->ncr.cerP->statusCode.code = (HttpStatusCode) atoi(node->value());

  return 0;
}



/* ****************************************************************************
*
* statusCodeReasonPhrase -
*/
static int statusCodeReasonPhrase(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode reasonPhrase: '%s'", node->value()));
  parseDataP->ncr.cerP->statusCode.reasonPhrase = node->value();  // OK - parsing step

  return 0;
}



/* ****************************************************************************
*
* statusCodeDetails -
*/
static int statusCodeDetails(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode details: '%s'", node->value()));
  parseDataP->ncr.cerP->statusCode.details = node->value();

  return 0;
}


/* ****************************************************************************
*
* ncrParseVector -
*/
#define NCR "/notifyContextRequest"
#define CRL "/contextResponseList"
#define CR  "/contextElementResponse"
#define CE  "/contextElement"
#define CAL "/contextAttributeList"
#define CA  "/contextAttribute"
#define MDL "/metadata"
#define MD  "/contextMetadata"
#define DMD "/domainMetadata"
#define ST  "/statusCode"

XmlNode ncrParseVector[] =
{
  { NCR,                                  nullTreat                    },
  { NCR "/subscriptionId",                subscriptionId               },
  { NCR "/originator",                    originator                   },
  { NCR "/contextResponseList",           nullTreat                    },

  { NCR CRL CR,                           contextElementResponse       },
  { NCR CRL CR CE,                        nullTreat                    },
  { NCR CRL CR CE "/entityId",            entityId                     },
  { NCR CRL CR CE "/entityId/id",         entityIdId                   },
  { NCR CRL CR CE "/attributeDomainName", attributeDomainName          },

  { NCR CRL CR CE CAL,                    nullTreat                    },
  { NCR CRL CR CE CAL CA,                 contextAttribute             },
  { NCR CRL CR CE CAL CA "/name",         contextAttributeName         },
  { NCR CRL CR CE CAL CA "/type",         contextAttributeType         },
  { NCR CRL CR CE CAL CA "/contextValue", contextAttributeContextValue },

  { NCR CRL CR CE CAL CA MDL,             nullTreat                    },
  { NCR CRL CR CE CAL CA MDL MD,          contextMetadata              },
  { NCR CRL CR CE CAL CA MDL MD "/name",  contextMetadataName          },
  { NCR CRL CR CE CAL CA MDL MD "/type",  contextMetadataType          },
  { NCR CRL CR CE CAL CA MDL MD "/value", contextMetadataValue         },

  { NCR CRL CR CE "/domainMetadata",      nullTreat                    },
  { NCR CRL CR CE DMD "/contextMetadata", nullTreat                    },
  { NCR CRL CR CE DMD MD "/name",         nullTreat                    },
  { NCR CRL CR CE DMD MD "/type",         nullTreat                    },
  { NCR CRL CR CE DMD MD "/value",        nullTreat                    },

  { NCR CRL CR "/statusCode",             nullTreat                    },
  { NCR CRL CR ST "/code",                statusCodeCode               },
  { NCR CRL CR ST "/reasonPhrase",        statusCodeReasonPhrase       },
  { NCR CRL CR ST "/details",             statusCodeDetails            },

  { "LAST", NULL }
};
