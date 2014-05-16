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
* UpdateContextResponse::~UpdateContextResponse -
*/
UpdateContextResponse::~UpdateContextResponse()
{
  errorCode.release();
  contextElementResponseVector.release();
  LM_T(LmtDestructor,("destroyed"));
}


/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse - 
*/
UpdateContextResponse::UpdateContextResponse(StatusCode& _errorCode)
{
  errorCode = _errorCode;

  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* UpdateContextResponse::render - 
*/
std::string UpdateContextResponse::render(RequestType requestType, Format format, const std::string& indent)
{
  std::string out = "";
  std::string tag = "updateContextResponse";

  out += startTag(indent, tag, format, false);

  if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
  {
    out += errorCode.render(format, indent + "  ");
  }
  else
  {
    if (contextElementResponseVector.size() == 0)
    {
      errorCode.fill(SccContextElementNotFound);
      out += errorCode.render(format, indent + "  ");
    }
    else
      out += contextElementResponseVector.render(UpdateContext, format, indent + "  ");
  }
  
  out += endTag(indent, tag, format);

  return out;
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
