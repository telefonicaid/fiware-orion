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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "convenience/UpdateContextAttributeRequest.h"
#include "ngsi/StatusCode.h"
#include "parse/compoundValue.h"



/* ****************************************************************************
*
* Constructor - 
*/
UpdateContextAttributeRequest::UpdateContextAttributeRequest()
{
  metadataVector.keyNameSet("metadata");
  compoundValueP = NULL;
  valueType = orion::ValueTypeNone;
}



/* ****************************************************************************
*
* render - 
*/
std::string UpdateContextAttributeRequest::render(ConnectionInfo* ciP, std::string indent)
{
  std::string tag = "updateContextAttributeRequest";
  std::string out = "";
  std::string indent2 = indent + "  ";
  bool        commaAfterContextValue = metadataVector.size() != 0;

  out += startTag1(indent, tag, false);
  out += valueTag1(indent2, "type", type, true);

  if (compoundValueP == NULL)
  {
    out += valueTag1(indent2, "contextValue", contextValue, true);
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    // FIXME PR
    out += startTag2(indent + "  ", "value", isCompoundVector, true);
    out += compoundValueP->render(ciP->apiVersion, indent + "    ");
    out += endTag(indent + "  ", commaAfterContextValue, isCompoundVector);
  }

  out += metadataVector.render(indent2);
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string UpdateContextAttributeRequest::check
(
  ConnectionInfo* ciP,
  RequestType     requestType,
  std::string     indent,
  std::string     predetectedError,
  int             counter
)
{
  StatusCode       response;
  std::string      res;

  indent = "  ";

  if (predetectedError != "")
  {
    response.fill(SccBadRequest, predetectedError);
  }
  // FIXME
  else if ((res = metadataVector.check(ciP->apiVersion)) != "OK")
  {
    response.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  std::string out = response.render(indent);

  out = "{\n" + out + "}\n";

  return out;
}



/* ****************************************************************************
*
* present - 
*/
void UpdateContextAttributeRequest::present(std::string indent)
{
  LM_T(LmtPresent, ("%stype:         %s", 
		    indent.c_str(), 
		    type.c_str()));
  LM_T(LmtPresent, ("%scontextValue: %s", 
		    indent.c_str(), 
		    contextValue.c_str()));
  metadataVector.present("ContextMetadata", indent);
}



/* ****************************************************************************
*
* release - 
*/
void UpdateContextAttributeRequest::release(void)
{
  metadataVector.release();

  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }
}
