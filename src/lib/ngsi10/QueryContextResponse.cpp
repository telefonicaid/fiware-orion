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
#include "rest/ConnectionInfo.h"



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
std::string QueryContextResponse::render(ConnectionInfo* ciP, RequestType requestType, const std::string& indent)
{
  std::string out = "";
  std::string tag = "queryContextResponse";

  out += startTag(indent, tag, ciP->outFormat, false);

  if (contextElementResponseVector.size() > 0)
  {
    bool commaNeeded = (errorCode.code != SccNone);
    out += contextElementResponseVector.render(ciP, QueryContext, indent + "  ", commaNeeded);
  }

  if (errorCode.code != SccNone)
  {
    out += errorCode.render(ciP->outFormat, indent + "  ");
  }

  /* Safety check: neither errorCode nor CER vector was filled by mongoBackend */
  if (errorCode.code == SccNone && contextElementResponseVector.size() == 0)
  {
      errorCode.fill(SccReceiverInternalError, "Both the error-code structure and the response vector were empty");
      out += errorCode.render(ciP->outFormat, indent + "  ");
  }

  out += endTag(indent, tag, ciP->outFormat);

  return out;
}
