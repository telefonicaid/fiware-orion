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
#include <string>
#include <vector>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/Format.h"
#include "common/globals.h"
#include "common/string.h"
#include "common/tag.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse - 
*/
UpdateContextResponse::UpdateContextResponse()
{
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse - 
*/
UpdateContextResponse::UpdateContextResponse(StatusCode& _errorCode)
{
  errorCode.fill(&_errorCode);
  errorCode.tagSet("errorCode");
  LM_T(LmtDestructor, ("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextResponse::~UpdateContextResponse -
*/
UpdateContextResponse::~UpdateContextResponse()
{
  errorCode.release();
//  errorCode.tagSet("errorCode");
  contextElementResponseVector.release();
  LM_T(LmtDestructor, ("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextResponse::render - 
*/
std::string UpdateContextResponse::render(ConnectionInfo* ciP, RequestType requestType, const std::string& indent)
{
  std::string out = "";
  std::string tag = "updateContextResponse";

  out += startTag(indent, tag, ciP->outFormat, false);

  if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
  {
    out += errorCode.render(ciP->outFormat, indent + "  ");
  }
  else
  {
    if (contextElementResponseVector.size() == 0)
    {
      errorCode.fill(SccContextElementNotFound);
      out += errorCode.render(ciP->outFormat, indent + "  ");
    }
    else
      out += contextElementResponseVector.render(ciP, UpdateContext, indent + "  ");
  }
  
  out += endTag(indent, tag, ciP->outFormat);

  return out;
}



/* ****************************************************************************
*
* UpdateContextResponse::check -
*/
std::string UpdateContextResponse::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string  res;

  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (contextElementResponseVector.check(UpdateContext, ciP->outFormat, indent, predetectedError, 0) != "OK")
  {
    LM_W(("Bad Input (%s)", res.c_str()));
    errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return render(ciP, UpdateContext, indent);
}



/* ****************************************************************************
*
* UpdateContextResponse::present -
*/
void UpdateContextResponse::present(const std::string& indent)
{
  contextElementResponseVector.present(indent + "  ");
  errorCode.present(indent + "  ");
}



/* ****************************************************************************
*
* UpdateContextResponse::release - 
*/
void UpdateContextResponse::release(void)
{
  LM_T(LmtRelease, ("In UpdateContextResponse::release"));
  contextElementResponseVector.release();
  errorCode.release();
}
