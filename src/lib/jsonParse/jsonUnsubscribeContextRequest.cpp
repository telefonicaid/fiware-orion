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
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonUnsubscribeContextRequest.h"



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));
  parseDataP->uncr.res.subscriptionId.set(value);
  return "OK";
}



/* ****************************************************************************
*
* jsonUncrParseVector -
*/
JsonNode jsonUncrParseVector[] =
{
  { "/subscriptionId", subscriptionId },
  { "LAST",            NULL           }
};



/* ****************************************************************************
*
* jsonUncrInit - 
*/
void jsonUncrInit(ParseData* parseDataP)
{
  jsonUncrRelease(parseDataP);
  parseDataP->errorString  = "";
}



/* ****************************************************************************
*
* jsonUncrRelease - 
*/
void jsonUncrRelease(ParseData* parseDataP)
{
  parseDataP->uncr.res.release();
}



/* ****************************************************************************
*
* jsonUncrCheck - 
*/
std::string jsonUncrCheck(ParseData* parseData, ConnectionInfo* ciP)
{
  return parseData->uncr.res.check();
}
