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
#include "jsonParse/jsonUnsubscribeContextAvailabilityRequest.h"



/* ****************************************************************************
*
* subscriptionId - 
*/
static std::string subscriptionId(const std::string& path, const std::string& value, ParseData* parseDataP)
{
  LM_T(LmtParse, ("%s: %s", path.c_str(), value.c_str()));
  parseDataP->ucar.res.subscriptionId.set(value);
  return "OK";
}



/* ****************************************************************************
*
* jsonUcarParseVector -
*/
JsonNode jsonUcarParseVector[] =
{
  { "/subscriptionId",  subscriptionId },
  { "LAST",             NULL }
};



/* ****************************************************************************
*
* jsonUcarInit - 
*/
void jsonUcarInit(ParseData* parseDataP)
{
  jsonUcarRelease(parseDataP);
  parseDataP->errorString = "";
}



/* ****************************************************************************
*
* jsonUcarRelease - 
*/
void jsonUcarRelease(ParseData* parseDataP)
{
  parseDataP->ucar.res.release();
}



/* ****************************************************************************
*
* jsonUcarCheck - 
*/
std::string jsonUcarCheck(ParseData* parseData, ConnectionInfo* ciP)
{
  return parseData->ucar.res.check(parseData->errorString);
}



/* ****************************************************************************
*
* jsonUcarPresent - 
*/
void jsonUcarPresent(ParseData* parseDataP)
{
  if (!lmTraceIsSet(LmtPresent))
    return;

  LM_T(LmtPresent, ("\n\n"));

  parseDataP->ucar.res.subscriptionId.present("");
}
