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
#include <stdio.h>
#include <string>

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
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

  if ((parseDataP->rcr.entityIdP->id != "") && (parseDataP->rcr.entityIdP->id != node->value()))
    LM_W(("Overwriting entityId:id (was '%s') for '%s'", parseDataP->rcr.entityIdP->id.c_str(), node->value()));
  parseDataP->rcr.entityIdP->id = node->value();

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
  parseDataP->rcr.attributeMetadataP->value = node->value();
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
  parseDataP->rcr.registrationMetadataP->value = node->value();
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
* sourceEntityId - 
*/
static int sourceEntityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got a sourceEntityId"));

  LM_T(LmtParse, ("calling entityIdParse"));
  std::string es = entityIdParse(RegisterContext, node, &parseDataP->rcr.registrationMetadataP->association.entityAssociation.source);
  LM_T(LmtParse, ("back from  entityIdParse"));

  if (es != "OK")
  {
    parseDataP->errorString = es;
    LM_W(("Error parsing entity: %s", es.c_str()));
  }

  return 0;
}



/* ****************************************************************************
*
* sourceEntityIdId - 
*/
static int sourceEntityIdId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got a source entityId:id: '%s'", node->value()));

  parseDataP->rcr.registrationMetadataP->association.entityAssociation.source.id = node->value();

  return 0;
}



/* ****************************************************************************
*
* targetEntityId - 
*/
static int targetEntityId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got a targetEntityId"));

  std::string es = entityIdParse(RegisterContext, node, &parseDataP->rcr.registrationMetadataP->association.entityAssociation.target);

  if (es != "OK")
    parseDataP->errorString = es;

  return 0;
}



/* ****************************************************************************
*
* targetEntityIdId - 
*/
static int targetEntityIdId(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtParse, ("got a target entityId:id: '%s'", node->value()));

  parseDataP->rcr.registrationMetadataP->association.entityAssociation.target.id = node->value();

  return 0;
}



/* ****************************************************************************
*
* attributeAssociation - 
*/
static int attributeAssociation(xml_node<>* node, ParseData* parseDataP)
{
   LM_T(LmtParse, ("got an attribute association"));
   parseDataP->rcr.attributeAssociationP = new AttributeAssociation();

   parseDataP->rcr.registrationMetadataP->association.attributeAssociationList.push_back(parseDataP->rcr.attributeAssociationP);
   return 0;
}



/* ****************************************************************************
*
* sourceAttribute - 
*/
static int sourceAttribute(xml_node<>* node, ParseData* parseDataP)
{
   LM_T(LmtParse, ("got a source attribute association"));
   parseDataP->rcr.attributeAssociationP->source = node->value();
   return 0;
}



/* ****************************************************************************
*
* targetAttribute - 
*/
static int targetAttribute(xml_node<>* node, ParseData* parseDataP)
{
   LM_T(LmtParse, ("got a target attribute association"));
   parseDataP->rcr.attributeAssociationP->target = node->value();
   return 0;
}



/* ****************************************************************************
*
* entityIdList - 
*/
static int entityIdList(xml_node<>* node, ParseData* parseDataP)
{
   LM_T(LmtParse, ("got an entityIdList"));
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
  return parseDataP->rcr.res.check(RegisterContext, ciP->outFormat, "", parseDataP->errorString, 0);
}


#define PRINTF printf
/* ****************************************************************************
*
* rcrPresent - 
*/
void rcrPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  parseDataP->rcr.res.contextRegistrationVector.present("");
  parseDataP->rcr.res.duration.present("");
  parseDataP->rcr.res.registrationId.present("");
}



/* ****************************************************************************
*
* registerContextRequestParseVector - 
*/
XmlNode rcrParseVector[] = 
{
  { "/registerContextRequest",                         nullTreat },

  { "/registerContextRequest/contextRegistrationList", nullTreat },

  { "/registerContextRequest/contextRegistrationList/contextRegistration",                                   contextRegistration },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/entityIdList",                      entityIdList        },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/entityIdList/entityId",             entityId            },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/entityIdList/entityId/id",          entityIdId          },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList",  nullTreat           },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute",           crAttribute },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/name",      craName     },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/type",      craType     },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/isDomain",  craIsDomain },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/metadata",  nullTreat   },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/metadata/contextMetadata",       craMetadata      },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/metadata/contextMetadata/name",  craMetadataName  },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/metadata/contextMetadata/type",  craMetadataType  },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/contextRegistrationAttributeList/contextRegistrationAttribute/metadata/contextMetadata/value", craMetadataValue },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata",                       nullTreat            },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata",       regMetadata          },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/name",  regMetadataName      },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/type",  regMetadataType      },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value", regMetadataValue     },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/entityAssociation",      nullTreat },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/entityAssociation/sourceEntityId",    sourceEntityId    },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/entityAssociation/sourceEntityId/id", sourceEntityIdId  },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/entityAssociation/targetEntityId",    targetEntityId    },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/entityAssociation/targetEntityId/id", targetEntityIdId  },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/AttributeAssociationList",                                      nullTreat },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/AttributeAssociationList/AttributeAssociation",                 attributeAssociation},
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/AttributeAssociationList/AttributeAssociation/sourceAttribute", sourceAttribute },
  { "/registerContextRequest/contextRegistrationList/contextRegistration/registrationMetadata/contextMetadata/value/AttributeAssociationList/AttributeAssociation/targetAttribute", targetAttribute },

  { "/registerContextRequest/contextRegistrationList/contextRegistration/providingApplication",                       providingApplication },

  { "/registerContextRequest/duration",        duration       },

  { "/registerContextRequest/registrationId",  registrationId },

  { "LAST", NULL }
};
