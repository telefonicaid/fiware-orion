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
#include "ngsi10/UnsubscribeContextRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlUnsubscribeContextRequest.h"



/* ****************************************************************************
*
* subscriptionId -
*/
static int subscriptionId(xml_node<>* node, ParseData* reqData)
{
  reqData->uncr.res.subscriptionId.set(node->value());

  return 0;
}



/* ****************************************************************************
*
* uncrInit -
*/
void uncrInit(ParseData* reqData)
{
}



/* ****************************************************************************
*
* uncrRelease -
*/
void uncrRelease(ParseData* reqData)
{
  reqData->uncr.res.release();
}



/* ****************************************************************************
*
* uncrCheck -
*/
std::string uncrCheck(ParseData* reqData, ConnectionInfo* ciP)
{
  return reqData->uncr.res.check(UnsubscribeContext, ciP->outFormat, "", reqData->errorString, 0);
}



/* ****************************************************************************
*
* uncrPresent -
*/
void uncrPresent(ParseData* reqData)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  LM_T(LmtPresent, ("\n\n"));
  reqData->uncr.res.present("");
}



/* ****************************************************************************
*
* uncrParseVector -
*/
XmlNode uncrParseVector[] =
{
  { "/unsubscribeContextRequest",                 nullTreat       },
  { "/unsubscribeContextRequest/subscriptionId",  subscriptionId  },

  { "LAST", NULL }
};
