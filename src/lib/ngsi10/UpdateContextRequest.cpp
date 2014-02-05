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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ContextElement.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* UpdateContextRequest::init - 
*/
void UpdateContextRequest::init(void)
{
}



/* ****************************************************************************
*
* UpdateContextRequest::render - 
*/
std::string UpdateContextRequest::render(RequestType requestType, Format format, std::string indent)
{
  std::string  out = "";
  std::string  tag = "updateContextRequest";

  // JSON commas:
  // Both fields are MANDATORY, so, comma after "contextElementVector"
  //
  out += startTag(indent, tag, format, false);
  out += contextElementVector.render(UpdateContext, format, indent + "  ", true);
  out += updateActionType.render(format, indent + "  ", false);
  out += endTag(indent, tag, format, false);

  return out;
}



/* ****************************************************************************
*
* UpdateContextRequest::check - 
*/
std::string UpdateContextRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string            res;
  UpdateContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
    return response.render(UpdateContext, format, indent);
  }

  if (((res = contextElementVector.check(requestType, format, indent, predetectedError, counter)) != "OK") || 
      ((res = updateActionType.check(requestType, format, indent, predetectedError, counter)) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
    return response.render(UpdateContext, format, indent);
  }

  return "OK";
}



/* ****************************************************************************
*
* UpdateContextRequest::release - 
*/
void UpdateContextRequest::release(void)
{
  contextElementVector.release();
}



/* ****************************************************************************
*
* UpdateContextRequest::present - 
*/
void UpdateContextRequest::present(std::string indent)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  contextElementVector.present(indent);
  updateActionType.present(indent);
}
