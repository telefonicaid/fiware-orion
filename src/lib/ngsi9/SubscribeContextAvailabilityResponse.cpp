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
#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse - 
*/
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse()
{
  subscriptionId.set("");
  duration.set("");
  errorCode.fill(NO_ERROR_CODE, "", "");
}

/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::~SubscribeContextAvailabilityResponse -
*/
SubscribeContextAvailabilityResponse::~SubscribeContextAvailabilityResponse()
{
   LM_T(LmtDestructor,("destroyed"));
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse - 
*/
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse(std::string _subscriptionId, std::string _duration)
{
  subscriptionId.set(_subscriptionId);
  duration.set(_duration);
  errorCode.fill(NO_ERROR_CODE, "", "");
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse - 
*/
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse(std::string _subscriptionId, ErrorCode _errorCode)
{
  subscriptionId.set(_subscriptionId);
  errorCode     = _errorCode;
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::render - 
*/
std::string SubscribeContextAvailabilityResponse::render(RequestType requestType, Format format, std::string indent)
{
  std::string tag = "subscribeContextAvailabilityResponse";
  std::string out = "";

  out += startTag(indent, tag, format, false);
  out += subscriptionId.render(format, indent + "  ");

  if (!duration.isEmpty())
    out += duration.render(format, indent + "  ");

  if (errorCode.code != NO_ERROR_CODE)
    out += errorCode.render(format, indent + "  ");

  out += endTag(indent, tag, format);
  
  return out;
}  
