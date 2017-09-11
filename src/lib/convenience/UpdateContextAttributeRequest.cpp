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
#include "convenience/UpdateContextAttributeRequest.h"
#include "ngsi/StatusCode.h"
#include "parse/compoundValue.h"



/* ****************************************************************************
*
* Constructor - 
*/
UpdateContextAttributeRequest::UpdateContextAttributeRequest()
{
  compoundValueP = NULL;
  valueType = orion::ValueTypeNone;
}



/* ****************************************************************************
*
* render - 
*/
std::string UpdateContextAttributeRequest::render
(
  ApiVersion apiVersion,
  int        indent
)
{
  if (indent < 0) {
    indent = DEFAULT_JSON_INDENT_V1;
  }
  JsonHelper writer(indent);

  toJson(writer, apiVersion);

  return writer.str();
}



/* ****************************************************************************
*
* toJson - 
*/
void UpdateContextAttributeRequest::toJson
(
  JsonHelper& writer,
  ApiVersion apiVersion
)
{
  writer.StartObject();

  writer.String("type", type);

  if (compoundValueP == NULL)
  {
    writer.String("contextValue", contextValue);
  }
  else
  {
    writer.Key("value");
    compoundValueP->toJson(writer);
  }

  metadataVector.toJsonV1(writer);

  writer.EndObject();
}



/* ****************************************************************************
*
* check - 
*/
std::string UpdateContextAttributeRequest::check
(
  ApiVersion          apiVersion,
  std::string         indent,
  const std::string&  predetectedError
)
{
  StatusCode       response;
  std::string      res;

  if (predetectedError != "")
  {
    response.fill(SccBadRequest, predetectedError);
  }
  else if ((res = metadataVector.check(apiVersion)) != "OK")
  {
    response.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  JsonHelper writer(indent.length());
  writer.StartObject();
  response.toJsonV1(writer);
  writer.EndObject();
  return writer.str();
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
