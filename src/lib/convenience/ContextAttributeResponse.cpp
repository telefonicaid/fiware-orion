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

#include "logMsg/logMsg.h"

#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/StatusCode.h"
#include "convenience/ContextAttributeResponse.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* render - 
*/
std::string ContextAttributeResponse::render(ConnectionInfo* ciP, RequestType request, std::string indent)
{
  std::string tag = "contextAttributeResponse";
  std::string out = "";

  out += startTag(indent, tag, ciP->outFormat, false);
  out += contextAttributeVector.render(ciP, request, indent + "  ", true);
  out += statusCode.render(ciP->outFormat, indent + "  ");
  out += endTag(indent, tag, ciP->outFormat);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string ContextAttributeResponse::check
(
  ConnectionInfo*  ciP,
  RequestType      requestType,
  std::string      indent,
  std::string      predetectedError,
  int              counter
)
{
  std::string  res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeVector.check(requestType, ciP->outFormat, indent, predetectedError, counter)) != "OK")
  {
    LM_W(("Bad Input (contextAttributeVector: '%s')", res.c_str()));
    statusCode.fill(SccBadRequest, res);

    //
    // If this ContextAttributeResponse is part of an IndividualContextEntity, the complete rendered 
    // response is not desired, just the string returned from the check method
    //
    if (requestType == IndividualContextEntity)
    {
      return res;
    }
  }
  else 
  {
    return "OK";
  }

  return render(ciP, requestType, indent);
}



/* ****************************************************************************
*
* present - 
*/
void ContextAttributeResponse::present(std::string indent)
{
  contextAttributeVector.present(indent);
  statusCode.present(indent);
}



/* ****************************************************************************
*
* release - 
*/
void ContextAttributeResponse::release(void)
{
  contextAttributeVector.release();
}
