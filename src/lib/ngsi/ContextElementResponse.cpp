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
#include <string>

#include "logMsg/logMsg.h"

#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/ContextElementResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextElementResponse::render - 
*/
std::string ContextElementResponse::render
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  bool                comma
)
{
  std::string xmlTag   = "contextElementResponse";
  std::string jsonTag  = "contextElement";
  std::string out      = "";

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, false, false);
  out += contextElement.render(ciP, requestType, indent + "  ", true);
  out += statusCode.render(ciP->outFormat, indent + "  ", false);
  out += endTag(indent, xmlTag, ciP->outFormat, comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::release - 
*/
void ContextElementResponse::release(void)
{
  contextElement.release();
  statusCode.release();
}



/* ****************************************************************************
*
* ContextElementResponse::check - 
*/
std::string ContextElementResponse::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = contextElement.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = statusCode.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponse::present - 
*/
void ContextElementResponse::present(const std::string& indent, int ix)
{
  contextElement.present(indent, ix);
  statusCode.present(indent);
}
