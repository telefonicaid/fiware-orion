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
#include "ngsi/Request.h"
#include "convenience/RegisterProviderRequest.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlRegisterProviderRequest.h"



/* ****************************************************************************
*
* contextMetadata -
*/
static int contextMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata"));

  reqData->rpr.metadataP = new Metadata();
  reqData->rpr.res.metadataVector.push_back(reqData->rpr.metadataP);

  return 0;
}



/* ****************************************************************************
*
* contextMetadataName -
*/
static int contextMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name '%s'", node->value()));
  reqData->rpr.metadataP->name = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextMetadataType -
*/
static int contextMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type '%s'", node->value()));
  reqData->rpr.metadataP->type = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue -
*/
static int contextMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value '%s'", node->value()));
  reqData->rpr.metadataP->stringValue = node->value();

  return 0;
}



/* ****************************************************************************
*
* duration -
*/
static int duration(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a duration '%s'", node->value()));
  reqData->rpr.res.duration.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* providingApplication -
*/
static int providingApplication(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a providingApplication '%s'", node->value()));
  reqData->rpr.res.providingApplication.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* registrationId -
*/
static int registrationId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a registrationId '%s'", node->value()));
  reqData->rpr.res.registrationId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* rprParseVector -
*/
XmlNode rprParseVector[] =
{
  { "/registerProviderRequest",                                 nullTreat             },

  { "/registerProviderRequest/metadata",                        nullTreat             },
  { "/registerProviderRequest/metadata/contextMetadata",        contextMetadata       },
  { "/registerProviderRequest/metadata/contextMetadata/name",   contextMetadataName   },
  { "/registerProviderRequest/metadata/contextMetadata/type",   contextMetadataType   },
  { "/registerProviderRequest/metadata/contextMetadata/value",  contextMetadataValue  },

  { "/registerProviderRequest/duration",                        duration              },
  { "/registerProviderRequest/providingApplication",            providingApplication  },
  { "/registerProviderRequest/registrationId",                  registrationId        },

  { "LAST", NULL }
};



/* ****************************************************************************
*
* rprInit -
*/
void rprInit(ParseData* reqData)
{
  reqData->rpr.res.registrationId.set("");
  reqData->rpr.res.providingApplication.set("");
  reqData->rpr.res.duration.set("");

  reqData->rpr.metadataP = NULL;
}



/* ****************************************************************************
*
* rprRelease -
*/
void rprRelease(ParseData* reqData)
{
  reqData->rpr.res.release();
}



/* ****************************************************************************
*
* rprCheck -
*/
std::string rprCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->rpr.res.check(ciP, ContextEntitiesByEntityId, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* rprPresent -
*/
void rprPresent(ParseData* reqData)
{
  reqData->rpr.res.present("");
}
