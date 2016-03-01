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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UnsubscribeContextResponse.h"

/* ****************************************************************************
*
* UnsubscribeContextResponse::UnsubscribeContextResponse -
*/
UnsubscribeContextResponse::UnsubscribeContextResponse()
{
   LM_T(LmtDestructor,("created"));
}



/* ****************************************************************************
*
* UnsubscribeContextResponse::UnsubscribeContextResponse -
*/
UnsubscribeContextResponse::UnsubscribeContextResponse(StatusCode& _statusCode)
{
   statusCode.fill(&_statusCode);
   subscriptionId.set("000000000000000000000000");
}



/* ****************************************************************************
*
* UnsubscribeContextResponse::~UnsubscribeContextResponse -
*/
UnsubscribeContextResponse::~UnsubscribeContextResponse()
{
  LM_T(LmtDestructor,("destroyed"));
}

/* ****************************************************************************
*
* UnsubscribeContextResponse::render - 
*/
std::string UnsubscribeContextResponse::render(RequestType requestType, Format format, const std::string& indent)
{
  std::string out = "";
  std::string tag = "unsubscribeContextResponse";

  //out += startTag(indent, tag, format, false);
  out += startTag1(indent, tag, false);
  out += subscriptionId.render(RtUnsubscribeContextResponse, format, indent + "  ", true);
  out += statusCode.render(format, indent + "  ", false);
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* UnsubscribeContextResponse::release - 
*/
void UnsubscribeContextResponse::release(void)
{
}
