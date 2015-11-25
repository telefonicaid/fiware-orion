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
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/EntityId.h"
#include "ngsi9/RegisterContextResponse.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlRegisterContextResponse.h"



/* ****************************************************************************
*
* duration - 
*/
static int duration(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a duration: '%s'", node->value()));
  reqData->rcrs.res.duration.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* registrationId - 
*/
static int registrationId(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a registration id: '%s'", node->value()));
  reqData->rcrs.res.registrationId.set(node->value());
  return 0;
}



/* ****************************************************************************
*
* errorCodeCode - 
*/
static int errorCodeCode(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got a errorCode code: '%s'", node->value()));
  reqData->rcrs.res.errorCode.code = (HttpStatusCode) atoi(node->value());
  return 0;
}



/* ****************************************************************************
*
* errorCodeReasonPhrase - 
*/
static int errorCodeReasonPhrase(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an errorCode reason phrase: '%s'", node->value()));
  reqData->rcrs.res.errorCode.reasonPhrase = node->value();  // OK - parsing step
  return 0;
}



/* ****************************************************************************
*
* errorCodeDetails - 
*/
static int errorCodeDetails(xml_node<>* node, ParseData* reqData)
{
  LM_T(LmtParse, ("Got an errorCode details string: '%s'", node->value()));
  reqData->rcrs.res.errorCode.details = node->value();
  return 0;
}



/* ****************************************************************************
*
* registerContextResponseParseVector - 
*/
XmlNode rcrsParseVector[] =
{
  { "/registerContextResponse",                         nullTreat              },
  { "/registerContextResponse/duration",                duration               },
  { "/registerContextResponse/registrationId",          registrationId         },
  { "/registerContextResponse/errorCode",               nullTreat              },
  { "/registerContextResponse/errorCode/code",          errorCodeCode          },
  { "/registerContextResponse/errorCode/reasonPhrase",  errorCodeReasonPhrase  },
  { "/registerContextResponse/errorCode/details",       errorCodeDetails       },


  { "LAST", NULL }
};



/* ****************************************************************************
*
* rcrsInit - 
*/
void rcrsInit(ParseData* reqData)
{
  reqData->errorString = "";
}



/* ****************************************************************************
*
* rcrsRelease - 
*/
void rcrsRelease(ParseData* reqData)
{
}



/* ****************************************************************************
*
* rcrsCheck - 
*/
std::string rcrsCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->rcrs.res.check(RegisterContext, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* rcrsPresent - 
*/
void rcrsPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  LM_T(LmtPresent,("\n\n"));
  reqData->rcrs.res.duration.present("");
  reqData->rcrs.res.registrationId.present("");
  reqData->rcrs.res.errorCode.present("");
}



