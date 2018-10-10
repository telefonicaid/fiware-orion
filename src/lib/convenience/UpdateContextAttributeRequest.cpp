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
  compoundValueP = NULL;
  valueType = orion::ValueTypeNotGiven;
}



/* ****************************************************************************
*
* toJsonV1 -
*/
std::string UpdateContextAttributeRequest::toJsonV1(void)
{
  std::string out = "";

  out += startTag();
  out += valueTag("type", type, true);

  if (compoundValueP == NULL)
  {
    out += valueTag("contextValue", contextValue, true);
  }
  else
  {
    out += JSON_STR("value") + ":" + compoundValueP->toJson();
  }

  out += metadataVector.toJsonV1(metadataVector.vec, false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string UpdateContextAttributeRequest::check
(
  ApiVersion          apiVersion,
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

  std::string out = response.toJsonV1(false);

  out = "{" + out + "}";

  return out;
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
