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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi9/RegisterContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlRegisterContextRequest.h"



/* ****************************************************************************
*
* contextRegistration -
*/
static int contextRegistration(xml_node<>* node, ParseData* parseDataP)
{
  parseDataP->rcr.crP = new ContextRegistration();
  parseDataP->rcr.res.contextRegistrationVector.push_back(parseDataP->rcr.crP);

  return 0;
}



/* ****************************************************************************
*
* entityId -
*/
static int entityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an entityId"));

  parseDataP->rcr.entityIdP = new EntityId();
  parseDataP->rcr.crP->entityIdVector.push_back(parseDataP->rcr.entityIdP);

  std::string es = entityIdParse(RegisterContext, node, parseDataP->rcr.entityIdP);

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

  if (parseDataP->rcr.entityIdP != NULL)
  {
    parseDataP->rcr.entityIdP->id = node->value();
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
* crAttribute -
*/
static int crAttribute(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating an attribute"));

  parseDataP->rcr.attributeP = new ContextRegistrationAttribute();
  parseDataP->rcr.crP->contextRegistrationAttributeVector.push_back(parseDataP->rcr.attributeP);

  return 0;
}



/* ****************************************************************************
*
* craName -
*/
static int craName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute name: '%s'", node->value()));
  parseDataP->rcr.attributeP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* craType -
*/
static int craType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got an attribute type: '%s'", node->value()));
  parseDataP->rcr.attributeP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* craIsDomain -
*/
static int craIsDomain(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a isDomain: '%s'", node->value()));
  parseDataP->rcr.attributeP->isDomain = node->value();

  return 0;
}



/* ****************************************************************************
*
* craMetadata -
*/
static int craMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating a metadata"));

  parseDataP->rcr.attributeMetadataP = new Metadata();
  parseDataP->rcr.attributeP->metadataVector.push_back(parseDataP->rcr.attributeMetadataP);

  return 0;
}



/* ****************************************************************************
*
* craMetadataName -
*/
static int craMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", node->value()));
  parseDataP->rcr.attributeMetadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* craMetadataType -
*/
static int craMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", node->value()));
  parseDataP->rcr.attributeMetadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* craMetadataValue -
*/
static int craMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", node->value()));
  parseDataP->rcr.attributeMetadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* regMetadata -
*/
static int regMetadata(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Creating a reg metadata"));

  parseDataP->rcr.registrationMetadataP = new Metadata();
  parseDataP->rcr.crP->registrationMetadataVector.push_back(parseDataP->rcr.registrationMetadataP);

  return 0;
}



/* ****************************************************************************
*
* regMetadataName -
*/
static int regMetadataName(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a reg metadata name: '%s'", node->value()));
  parseDataP->rcr.registrationMetadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* regMetadataType -
*/
static int regMetadataType(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a reg metadata type: '%s'", node->value()));
  parseDataP->rcr.registrationMetadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* regMetadataValue -
*/
static int regMetadataValue(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a reg metadata value: '%s'", node->value()));
  parseDataP->rcr.registrationMetadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* providingApplication -
*/
static int providingApplication(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a providing application: '%s'", node->value()));
  parseDataP->rcr.crP->providingApplication.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* duration -
*/
static int duration(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  parseDataP->rcr.res.duration.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* registrationId -
*/
static int registrationId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("Got a registration id: '%s'", node->value()));
  parseDataP->rcr.res.registrationId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* entityIdList -
*/
static int entityIdList(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got an entityIdList"));

  if (parseDataP->rcr.crP->entityIdVectorPresent == true)
  {
    parseDataP->errorString = "Got an entityIdList when one was present already";
    alarmMgr.badInput(clientIp, "more than one list of entityId");
    return 1;
  }

  parseDataP->rcr.crP->entityIdVectorPresent = true;

  return 0;
}



/* ****************************************************************************
*
* rcrInit -
*/
void rcrInit(ParseData* parseDataP)
{
  parseDataP->rcr.crP                    = NULL;
  parseDataP->rcr.entityIdP              = NULL;
  parseDataP->rcr.attributeP             = NULL;
  parseDataP->rcr.attributeMetadataP     = NULL;
  parseDataP->rcr.registrationMetadataP  = NULL;
  parseDataP->errorString                = "";
}



/* ****************************************************************************
*
* rcrRelease -
*/
void rcrRelease(ParseData* parseDataP)
{
  parseDataP->rcr.res.release();
}



/* ****************************************************************************
*
* rcrCheck -
*/
std::string rcrCheck(ParseData* parseDataP, ConnectionInfo* ciP)
{
  return parseDataP->rcr.res.check(ciP, RegisterContext, ciP->outFormat, "", parseDataP->errorString, 0);
}


/* ****************************************************************************
*
* rcrPresent -
*/
void rcrPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
  {
    return;
  }

  LM_T(LmtPresent, ("\n\n"));
  parseDataP->rcr.res.contextRegistrationVector.present("");
  parseDataP->rcr.res.duration.present("");
  parseDataP->rcr.res.registrationId.present("");
}



/* ****************************************************************************
*
* registerContextRequestParseVector -
*/
#define RCR   "/registerContextRequest"
#define CRL   "/contextRegistrationList"
#define CR    "/contextRegistration"
#define EL    "/entityIdList"
#define CRAL  "/contextRegistrationAttributeList"
#define CRA   "/contextRegistrationAttribute"
#define MDL   "/metadata"
#define MD    "/contextMetadata"
#define RMDL  "/registrationMetadata"
#define VAL   "/value"

XmlNode rcrParseVector[] =
{
  { RCR "",                                            nullTreat            },

  { RCR CRL "",                                        nullTreat            },

  { RCR CRL CR "",                                     contextRegistration  },

  { RCR CRL CR EL "",                                  entityIdList         },
  { RCR CRL CR EL "/entityId",                         entityId             },
  { RCR CRL CR EL "/entityId/id",                      entityIdId           },

  { RCR CRL CR CRAL "",                                nullTreat            },

  { RCR CRL CR CRAL CRA,                               crAttribute          },
  { RCR CRL CR CRAL CRA "/name",                       craName              },
  { RCR CRL CR CRAL CRA "/type",                       craType              },
  { RCR CRL CR CRAL CRA "/isDomain",                   craIsDomain          },
  { RCR CRL CR CRAL CRA MDL,                           nullTreat            },
  { RCR CRL CR CRAL CRA MDL MD,                        craMetadata          },
  { RCR CRL CR CRAL CRA MDL MD "/name",                craMetadataName      },
  { RCR CRL CR CRAL CRA MDL MD "/type",                craMetadataType      },
  { RCR CRL CR CRAL CRA MDL MD "/value",               craMetadataValue     },

  { RCR CRL CR RMDL "",                                nullTreat            },
  { RCR CRL CR RMDL MD "",                             regMetadata          },
  { RCR CRL CR RMDL MD "/name",                        regMetadataName      },
  { RCR CRL CR RMDL MD "/type",                        regMetadataType      },
  { RCR CRL CR RMDL MD "/value",                       regMetadataValue     },

  { RCR CRL CR "/providingApplication",                providingApplication },

  { RCR "/duration",                                   duration             },
  { RCR "/registrationId",                             registrationId       },

  { "LAST", NULL }
};
