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
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUnsubscribeContextAvailabilityRequest.h"



/* ****************************************************************************
*
* subscriptionId -
*/
static int subscriptionId(xml_node<>* node, ParseData* reqData)
{
  reqData->ucar.res.subscriptionId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* ucarInit -
*/
void ucarInit(ParseData* reqData)
{
}



/* ****************************************************************************
*
* ucarRelease -
*/
void ucarRelease(ParseData* reqData)
{
  reqData->ucar.res.release();
}



/* ****************************************************************************
*
* ucarCheck -
*/
std::string ucarCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->ucar.res.check(UnsubscribeContextAvailability, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* ucarPresent -
*/
void ucarPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  LM_T(LmtDump, ("\n\n"));
  reqData->ucar.res.subscriptionId.present("");
}



/* ****************************************************************************
*
* ucarParseVector -
*/
XmlNode ucarParseVector[] =
{
  { "/unsubscribeContextAvailabilityRequest",                nullTreat      },
  { "/unsubscribeContextAvailabilityRequest/subscriptionId", subscriptionId },

  { "LAST", NULL }
};
