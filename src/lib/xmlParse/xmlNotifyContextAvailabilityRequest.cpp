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
#include "ngsi9/NotifyContextAvailabilityRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlNotifyContextAvailabilityRequest.h"



/* ****************************************************************************
*
* ncarInit - 
*/
void ncarInit(ParseData* parseDataP)
{
  ncarRelease(parseDataP);

  parseDataP->ncar.crrP                = NULL;
  parseDataP->ncar.entityIdP           = NULL;
  parseDataP->ncar.craP                = NULL;
  parseDataP->ncar.attributeMetadataP  = NULL;
  parseDataP->ncar.regMetadataP        = NULL;
}



/* ****************************************************************************
*
* ncarRelease - 
*/
void ncarRelease(ParseData* parseDataP)
{
  parseDataP->ncar.res.release();
}



/* ****************************************************************************
*
* ncarCheck - 
*/
std::string ncarCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->ncar.res.check(NotifyContextAvailability, ciP->outFormat, "", parseDataP->errorString, 0);
}



/* ****************************************************************************
*
* ncarPresent - 
*/
void ncarPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

  LM_T(LmtPresent,("\n\n"));
  parseDataP->ncar.res.present("");
}



/* ****************************************************************************
*
* subscriptionId - 
*/
static int subscriptionId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a subscriptionId: '%s'", node->value()));

  parseDataP->ncar.res.subscriptionId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* contextRegistrationResponse - 
*/
static int contextRegistrationResponse(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationResponse"));

  parseDataP->ncar.crrP = new ContextRegistrationResponse();
  parseDataP->ncar.res.contextRegistrationResponseVector.push_back(parseDataP->ncar.crrP);

  return 0;
}



/* ****************************************************************************
*
* entityId - 
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  parseDataP->ncar.entityIdP = new EntityId();
  parseDataP->ncar.crrP->contextRegistration.entityIdVector.push_back(parseDataP->ncar.entityIdP);

  std::string es = entityIdParse(NotifyContextAvailability, node, parseDataP->ncar.entityIdP);

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
  LM_T(LmtParse, ("Got an entityIdId: '%s'", node->value()));

  if (parseDataP->ncar.entityIdP != NULL)
  {
    parseDataP->ncar.entityIdP->id = node->value();
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
* contextRegistrationAttribute - 
*/
static int contextRegistrationAttribute(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationAttribute"));

  parseDataP->ncar.craP = new ContextRegistrationAttribute();
  parseDataP->ncar.crrP->contextRegistration.contextRegistrationAttributeVector.push_back(parseDataP->ncar.craP);

  return 0;
}



/* ****************************************************************************
*
* contextRegistrationAttributeName - 
*/
static int contextRegistrationAttributeName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationAttributeName: '%s'", node->value()));
  parseDataP->ncar.craP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextRegistrationAttributeType - 
*/
static int contextRegistrationAttributeType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationAttributeType: '%s'", node->value()));
  parseDataP->ncar.craP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextRegistrationAttributeIsDomain - 
*/
static int contextRegistrationAttributeIsDomain(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a contextRegistrationAttributeIsDomain: '%s'", node->value()));
  parseDataP->ncar.craP->isDomain = node->value();

  return 0;
}



/* ****************************************************************************
*
* attributeMetadata - 
*/
static int attributeMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadata: '%s'", node->value()));

  parseDataP->ncar.attributeMetadataP = new Metadata();
  parseDataP->ncar.craP->metadataVector.push_back(parseDataP->ncar.attributeMetadataP);

  return 0;
}



/* ****************************************************************************
*
* attributeMetadataName - 
*/
static int attributeMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataName: '%s'", node->value()));
  parseDataP->ncar.attributeMetadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* attributeMetadataType - 
*/
static int attributeMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataType: '%s'", node->value()));
  parseDataP->ncar.attributeMetadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* attributeMetadataValue - 
*/
static int attributeMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attributeMetadataValue: '%s'", node->value()));
  parseDataP->ncar.attributeMetadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* registrationMetadata - 
*/
static int registrationMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadata: '%s'", node->value()));
  parseDataP->ncar.regMetadataP = new Metadata();
  parseDataP->ncar.crrP->contextRegistration.registrationMetadataVector.push_back(parseDataP->ncar.regMetadataP);

  return 0;
}



/* ****************************************************************************
*
* registrationMetadataName - 
*/
static int registrationMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataName: '%s'", node->value()));
  parseDataP->ncar.regMetadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* registrationMetadataType - 
*/
static int registrationMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataType: '%s'", node->value()));
  parseDataP->ncar.regMetadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* registrationMetadataValue - 
*/
static int registrationMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registrationMetadataValue: '%s'", node->value()));
  parseDataP->ncar.regMetadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* providingApplication - 
*/
static int providingApplication(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a providingApplication: '%s'", node->value()));
  parseDataP->ncar.crrP->contextRegistration.providingApplication.set(node->value());

  return 0;
}


#define CRR "/notifyContextAvailabilityRequest/contextRegistrationResponseList/contextRegistrationResponse"
#define CRA CRR "/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute"
/* ****************************************************************************
*
* ncarParseVector - 
*/
XmlNode ncarParseVector[] =
{
  { "/notifyContextAvailabilityRequest",                                    nullTreat                             },
  { "/notifyContextAvailabilityRequest/subscriptionId",                     subscriptionId                        },
  { "/notifyContextAvailabilityRequest/contextRegistrationResponseList",    nullTreat                             },

  { CRR,                                                                    contextRegistrationResponse           },
  { CRR "/contextRegistration",                                             nullTreat                             },
  { CRR "/contextRegistration/entityIdList",                                nullTreat                             },
  { CRR "/contextRegistration/entityIdList/entityId",                       entityId                              },
  { CRR "/contextRegistration/entityIdList/entityId/id",                    entityIdId                            },

  { CRR "/contextRegistration/contextRegistrationAttributeList",            nullTreat                             },
  { CRA,                                                                    contextRegistrationAttribute          },
  { CRA "/name",                                                            contextRegistrationAttributeName      },
  { CRA "/type",                                                            contextRegistrationAttributeType      },
  { CRA "/isDomain",                                                        contextRegistrationAttributeIsDomain  },

  { CRA "/metadata",                                                        nullTreat                             },
  { CRA "/metadata/contextMetadata",                                        attributeMetadata                     },
  { CRA "/metadata/contextMetadata/name",                                   attributeMetadataName                 },
  { CRA "/metadata/contextMetadata/type",                                   attributeMetadataType                 },
  { CRA "/metadata/contextMetadata/value",                                  attributeMetadataValue                },

  { CRR "/contextRegistration/registrationMetadata",                        nullTreat                             },
  { CRR "/contextRegistration/registrationMetadata/contextMetadata",        registrationMetadata                  },
  { CRR "/contextRegistration/registrationMetadata/contextMetadata/name",   registrationMetadataName              },
  { CRR "/contextRegistration/registrationMetadata/contextMetadata/type",   registrationMetadataType              },
  { CRR "/contextRegistration/registrationMetadata/contextMetadata/value",  registrationMetadataValue             },

  { CRR "/contextRegistration/providingApplication",                        providingApplication                  },

  { "LAST", NULL }
};
