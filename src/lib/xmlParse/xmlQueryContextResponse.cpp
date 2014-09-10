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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/areas.h"
#include "ngsi/Request.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi/Restriction.h"
#include "ngsi10/QueryContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlQueryContextResponse.h"
#include "xmlParse/xmlParse.h"

using namespace orion;



/* ****************************************************************************
*
* contextElementResponse - 
*/
static int contextElementResponse(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextElementResponse"));
  parseDataP->qcrs.cerP = new ContextElementResponse();
  parseDataP->qcrs.res.contextElementResponseVector.push_back(parseDataP->qcrs.cerP);
  return 0;
}



/* ****************************************************************************
*
* entityId - 
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  std::string es = entityIdParse(QueryContext, node, &parseDataP->qcrs.cerP->contextElement.entityId);
  
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

  parseDataP->qcrs.cerP->contextElement.entityId.id = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextAttribute - 
*/
static int contextAttribute(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute"));
  parseDataP->qcrs.attributeP = new ContextAttribute();
  parseDataP->qcrs.cerP->contextElement.contextAttributeVector.push_back(parseDataP->qcrs.attributeP);
  return 0;
}



/* ****************************************************************************
*
* contextAttributeName - 
*/
static int contextAttributeName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: %s", node->value()));
  parseDataP->qcrs.attributeP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeType - 
*/
static int contextAttributeType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: %s", node->value()));
  parseDataP->qcrs.attributeP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextAttributeValue - 
*/
static int contextAttributeValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute value: %s", node->value()));
  parseDataP->lastContextAttribute = parseDataP->qcrs.attributeP;
  parseDataP->lastContextAttribute->typeFromXmlAttribute = xmlTypeAttributeGet(node);
  parseDataP->qcrs.attributeP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadata - 
*/
static int contextMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata"));
  parseDataP->qcrs.metadataP = new Metadata();
  parseDataP->qcrs.attributeP->metadataVector.push_back(parseDataP->qcrs.metadataP);
  return 0;
}



/* ****************************************************************************
*
* contextMetadataName - 
*/
static int contextMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  parseDataP->qcrs.metadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataType - 
*/
static int contextMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  parseDataP->qcrs.metadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue - 
*/
static int contextMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  parseDataP->qcrs.metadataP->value = node->value();
  return 0;
}



/* ****************************************************************************
*
* statusCodeCode - 
*/
static int statusCodeCode(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode code: '%s'", node->value()));
  parseDataP->qcrs.cerP->statusCode.code = (HttpStatusCode) atoi(node->value());
  return 0;
}



/* ****************************************************************************
*
* statusCodeReasonPhrase - 
*/
static int statusCodeReasonPhrase(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode reasonPhrase: '%s'", node->value()));
  parseDataP->qcrs.cerP->statusCode.reasonPhrase = node->value(); // OK - parsing step
  return 0;
}



/* ****************************************************************************
*
* statusCodeDetails - 
*/
static int statusCodeDetails(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a statusCode details: '%s'", node->value()));
  parseDataP->qcrs.cerP->statusCode.details = node->value();
  return 0;
}



/* ****************************************************************************
*
* errorCodeCode - 
*/
static int errorCodeCode(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode code: '%s'", node->value()));
  parseDataP->qcrs.res.errorCode.code = (HttpStatusCode) atoi(node->value());
  return 0;
}



/* ****************************************************************************
*
* errorCodeReasonPhrase - 
*/
static int errorCodeReasonPhrase(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode reasonPhrase: '%s'", node->value()));
  parseDataP->qcrs.res.errorCode.reasonPhrase = node->value(); // OK - parsing step
  return 0;
}



/* ****************************************************************************
*
* errorCodeDetails - 
*/
static int errorCodeDetails(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a errorCode details: '%s'", node->value()));
  parseDataP->qcrs.res.errorCode.details = node->value();
  return 0;
}



/* ****************************************************************************
*
* qcrsParseVector - 
*/
XmlNode qcrsParseVector[] = 
{
  { "/queryContextResponse",                                             nullTreat             },

  { "/queryContextResponse/contextResponseList",                                                    nullTreat               },
  { "/queryContextResponse/contextResponseList/contextElementResponse",                             contextElementResponse  },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement",              nullTreat               },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/entityId",     entityId                },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/entityId/id",  entityIdId              },

  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList",                                nullTreat              },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute",               contextAttribute       },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/name",          contextAttributeName   },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/type",          contextAttributeType   },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/contextValue",  contextAttributeValue  },

  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/metadata",                        nullTreat             },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata",        contextMetadata       },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/name",   contextMetadataName   },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/type",   contextMetadataType   },
  { "/queryContextResponse/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/metadata/contextMetadata/value",  contextMetadataValue  },

  { "/queryContextResponse/contextResponseList/contextElementResponse/statusCode",               nullTreat                  },
  { "/queryContextResponse/contextResponseList/contextElementResponse/statusCode/code",          statusCodeCode             },
  { "/queryContextResponse/contextResponseList/contextElementResponse/statusCode/reasonPhrase",  statusCodeReasonPhrase     },
  { "/queryContextResponse/contextResponseList/contextElementResponse/statusCode/details",       statusCodeDetails          },

  { "/queryContextResponse/errorCode",                                   nullTreat             },
  { "/queryContextResponse/errorCode/code",                              errorCodeCode         },
  { "/queryContextResponse/errorCode/reasonPhrase",                      errorCodeReasonPhrase },
  { "/queryContextResponse/errorCode/details",                           errorCodeDetails      },
  { "LAST", NULL }
};



/* ****************************************************************************
*
* qcrsInit - 
*/
void qcrsInit(ParseData* parseDataP)
{
  qcrsRelease(parseDataP);

  parseDataP->errorString = "";
}



/* ****************************************************************************
*
* qcrsRelease - 
*/
void qcrsRelease(ParseData* parseDataP)
{
  parseDataP->qcrs.res.release();
}



/* ****************************************************************************
*
* qcrsCheck - 
*/
std::string qcrsCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->qcrs.res.check(ciP, QueryContext, "", parseDataP->errorString, 0);
}


#define PRINTF printf
/* ****************************************************************************
*
* qcrsPresent - 
*/
void qcrsPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  parseDataP->qcrs.res.present("");
}
