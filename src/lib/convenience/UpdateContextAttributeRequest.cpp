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
#include "common/Format.h"
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
  metadataVector.tagSet("metadata");
  compoundValueP = NULL;
  valueType = orion::ValueTypeNone;
}



/* ****************************************************************************
*
* render - 
*/
std::string UpdateContextAttributeRequest::render(ConnectionInfo* ciP, Format format, std::string indent)
{
  std::string tag = "updateContextAttributeRequest";
  std::string out = "";
  std::string indent2 = indent + "  ";
  bool        commaAfterContextValue = metadataVector.size() != 0;

  out += startTag(indent, tag, format, false);
  out += valueTag(indent2, "type", type, format, true);

  if (compoundValueP == NULL)
  {
    out += valueTag(indent2, "contextValue", contextValue, format, true);
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag(indent + "  ", "contextValue", "value", format, isCompoundVector, true, isCompoundVector);
    out += compoundValueP->render(ciP, format, indent + "    ");
    out += endTag(indent + "  ", "contextValue", format, commaAfterContextValue, isCompoundVector);
  }

  out += metadataVector.render(format, indent2);
  out += endTag(indent, tag, format);

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
  Format          format,
  std::string     indent,
  std::string     predetectedError,
  int             counter
)
{
  StatusCode       response;
  std::string      res;

  if (format == (Format) 0)
  {
    format = XML;
  }

  if (format == JSON)
  {
    indent = "  ";
  }

  if (predetectedError != "")
  {
    response.fill(SccBadRequest, predetectedError);
  }
  else if ((res = metadataVector.check(ciP, requestType, format, indent, predetectedError, counter)) != "OK")
  {
    response.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  std::string out = response.render(format, indent);

  if (format == JSON)
  {
    out = "{\n" + out + "}\n";
  }

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
