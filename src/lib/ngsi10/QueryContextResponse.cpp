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
#include "rest/HttpStatusCode.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/QueryContextResponse.h"



/* ****************************************************************************
*
* QueryContextResponse::QueryContextResponse - 
*/
QueryContextResponse::QueryContextResponse()
{
  errorCode.tagSet("errorCode");
}


/* ****************************************************************************
*
* QueryContextResponse::QueryContextResponse - 
*/
QueryContextResponse::QueryContextResponse(StatusCode& _errorCode)
{
  errorCode.fill(&_errorCode);
  errorCode.tagSet("errorCode");
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
std::string QueryContextResponse::render(RequestType requestType, Format format, const std::string& indent)
{
  std::string out = "";
  std::string tag = "queryContextResponse";

  out += startTag(indent, tag, format, false);

  if (contextElementResponseVector.size() > 0)
  {
    bool commaNeeded = (errorCode.code != SccNone);
    out += contextElementResponseVector.render(QueryContext, format, indent + "  ", commaNeeded);
  }

  if (errorCode.code != SccNone)
  {
    out += errorCode.render(format, indent + "  ");
  }

  /* Safety check: neither errorCode nor CER vector was filled by mongoBackend */
  if (errorCode.code == SccNone && contextElementResponseVector.size() == 0)
  {
      errorCode.fill(SccReceiverInternalError, "Empty error and CER vector");
      out += errorCode.render(format, indent + "  ");
  }

#if 0
  // I needed to adjust rednder function for details=on to work. Ken, please review that this code can be safely removed, after the
  // above re-factoring
  if ((errorCode.code == SccNone) || (errorCode.code == SccOk))
  {
    if (contextElementResponseVector.size() == 0)
    {
      errorCode.fill(SccContextElementNotFound);
      out += errorCode.render(format, indent + "  ");
    }
    else 
    {
      out += contextElementResponseVector.render(QueryContext, format, indent + "  ");
    }
  }
  else
     out += errorCode.render(format, indent + "  ");
#endif

  out += endTag(indent, tag, format);

  return out;
}
