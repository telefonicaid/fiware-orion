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
  return parseDataP->ncr.res.check(NotifyContext, ciP->outFormat, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* ncrPresent - 
*/
void ncrPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
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

  parseDataP->ncr.cerP->contextElement.entityId.id = node->value();

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
  parseDataP->ncr.attributeP->value = node->value();
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
  parseDataP->ncr.cerP->statusCode.reasonPhrase = node->value();
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
XmlNode ncrParseVector[] = 
{
  { "/notifyContextRequest",                         nullTreat          },
  { "/notifyContextRequest/subscriptionId",          subscriptionId     },
  { "/notifyContextRequest/originator",              originator         },
  { "/notifyContextRequest/contextResponseList",     nullTreat          },

  { "/notifyContextRequest/contextResponseList/contextElementResponse",                             contextElementResponse  },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement",              nullTreat               },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/entityId",     entityId                },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/entityId/id",  entityIdId              },

  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList",                               nullTreat },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute",              contextAttribute },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/name",         contextAttributeName },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/type",         contextAttributeType },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/contextValue", contextAttributeContextValue },

  { "/notifyContextRequest/contextResponseList/contextElementResponse/statusCode",                nullTreat              },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/statusCode/code",           statusCodeCode         },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/statusCode/reasonPhrase",   statusCodeReasonPhrase },
  { "/notifyContextRequest/contextResponseList/contextElementResponse/statusCode/details",        statusCodeDetails      },
  
  { "LAST", NULL }
};
