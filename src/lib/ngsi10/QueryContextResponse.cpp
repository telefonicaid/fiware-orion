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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/string.h"
#include "common/tag.h"
#include "ngsi10/QueryContextResponse.h"



/* ****************************************************************************
*
* QueryContextResponse::QueryContextResponse - 
*/
QueryContextResponse::QueryContextResponse()
{
  errorCode.code         = NO_ERROR_CODE;
  errorCode.reasonPhrase = "";
  errorCode.details      = "";
}


/* ****************************************************************************
*
* QueryContextResponse::QueryContextResponse - 
*/
QueryContextResponse::QueryContextResponse(ErrorCode _errorCode)
{
  errorCode = _errorCode;
}

/* ****************************************************************************
*
* QueryContextResponse::~QueryContextResponse -
*/
QueryContextResponse::~QueryContextResponse()
{
  contextElementResponseVector.release();
  LM_T(LmtDestructor,("destroyed"));
}

/* ****************************************************************************
*
* QueryContextResponse::render - 
*/
std::string QueryContextResponse::render(RequestType requestType, Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "queryContextResponse";

  out += startTag(indent, tag, format, false);

  if ((errorCode.code == NO_ERROR_CODE) || (errorCode.code == SccOk))
  {
    if (contextElementResponseVector.size() == 0)
    {
      errorCode.fill(SccContextElementNotFound, "Query without hits", "");
      out += errorCode.render(format, indent + "  ");
    }
    else 
      out += contextElementResponseVector.render(format, indent + "  ");
  }
  else
     out += errorCode.render(format, indent + "  ");

  out += endTag(indent, tag, format);

  return out;
}
