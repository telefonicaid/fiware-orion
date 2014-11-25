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

#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ContextElement.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"



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
std::string UpdateContextRequest::render(ConnectionInfo* ciP, RequestType requestType, const std::string& indent)
{
  std::string  out = "";
  std::string  tag = "updateContextRequest";

  // JSON commas:
  // Both fields are MANDATORY, so, comma after "contextElementVector"
  //
  out += startTag(indent, tag, ciP->outFormat, false);
  out += contextElementVector.render(ciP, UpdateContext, indent + "  ", true);
  out += updateActionType.render(ciP->outFormat, indent + "  ", false);
  out += endTag(indent, tag, ciP->outFormat, false);

  return out;
}



/* ****************************************************************************
*
* UpdateContextRequest::check - 
*/
std::string UpdateContextRequest::check(ConnectionInfo* ciP, RequestType requestType, const std::string& indent, const std::string& predetectedError, int counter)
{
  std::string            res;
  UpdateContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
    return response.render(ciP, UpdateContext, indent);
  }

  if (((res = contextElementVector.check(requestType, ciP->outFormat, indent, predetectedError, counter)) != "OK") || 
      ((res = updateActionType.check(requestType,     ciP->outFormat, indent, predetectedError, counter)) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
    return response.render(ciP, UpdateContext, indent);
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
void UpdateContextRequest::present(const std::string& indent)
{
  if (!lmTraceIsSet(LmtDump))
    return;

  PRINTF("\n\n");
  contextElementVector.present(indent);
  updateActionType.present(indent);
}
