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
#include "convenience/UpdateContextAttributeRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUpdateContextAttributeRequest.h"



/* ****************************************************************************
*
* upcarInit -
*/
void upcarInit(ParseData* reqData)
{
}



/* ****************************************************************************
*
* upcarRelease -
*/
void upcarRelease(ParseData* reqData)
{
  reqData->upcar.res.release();
}



/* ****************************************************************************
*
* upcarCheck -
*/
std::string upcarCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->upcar.res.check(UpdateContextAttribute, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* upcarPresent -
*/
void upcarPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  LM_T(LmtDump, ("\n\n"));
  reqData->upcar.res.present("");
}



/* ****************************************************************************
*
* attributeType -
*/
static int attributeType(xml_node<>* node, ParseData* reqData)
{
  reqData->upcar.res.type = node->value();

  return 0;
}



/* ****************************************************************************
*
* attributeValue -
*/
static int attributeValue(xml_node<>* node, ParseData* reqData)
{
  //
  // NOTE: UpdateContextAttributeRequest doesn't have a ContextAttribute (it *is* an attribute)
  //       so no attributeP can exist in UpdateContextAttributeData. 
  //       However, in order to save the 'typeFromXmlAttribute', a typeFromXmlAttribute has been added to 
  //       UpdateContextAttributeData, and, in the function compoundValueEnd, instead of following 
  //       ParseData::lastContextAttribute to find the attribute in question, we go
  //       directly to ParseData::UpdateContextAttributeData::res, finding compoundValueP
  //
  reqData->upcar.res.typeFromXmlAttribute = xmlTypeAttributeGet(node);
  reqData->upcar.res.valueType            = orion::ValueTypeString;
  reqData->upcar.res.contextValue         = node->value();

  return 0;
}



/* ****************************************************************************
*
* contextMetadata -
*/
static int contextMetadata(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Creating a metadata"));
  reqData->upcar.metadataP = new Metadata();
  reqData->upcar.res.metadataVector.push_back(reqData->upcar.metadataP);
  return 0;
}



/* ****************************************************************************
*
* contextMetadataName -
*/
static int contextMetadataName(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata name: '%s'", node->value()));
  reqData->upcar.metadataP->name = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataType -
*/
static int contextMetadataType(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata type: '%s'", node->value()));
  reqData->upcar.metadataP->type = node->value();
  return 0;
}



/* ****************************************************************************
*
* contextMetadataValue -
*/
static int contextMetadataValue(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a metadata value: '%s'", node->value()));
  reqData->upcar.metadataP->stringValue = node->value();
  return 0;
}



/* ****************************************************************************
*
* upcarParseVector -
*/
XmlNode upcarParseVector[] =
{
  { "/updateContextAttributeRequest",                                 nullTreat             },
  { "/updateContextAttributeRequest/type",                            attributeType         },
  { "/updateContextAttributeRequest/contextValue",                    attributeValue        },
  { "/updateContextAttributeRequest/metadata",                        nullTreat             },
  { "/updateContextAttributeRequest/metadata/contextMetadata",        contextMetadata       },
  { "/updateContextAttributeRequest/metadata/contextMetadata/name",   contextMetadataName   },
  { "/updateContextAttributeRequest/metadata/contextMetadata/type",   contextMetadataType   },
  { "/updateContextAttributeRequest/metadata/contextMetadata/value",  contextMetadataValue  },

  { "LAST", NULL }
};
