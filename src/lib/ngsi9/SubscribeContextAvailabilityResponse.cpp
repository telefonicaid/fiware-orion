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
#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse -
*/
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse() : errorCode("errorCode")
{
  subscriptionId.set("");
  duration.set("");
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
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse(const std::string& _subscriptionId, const std::string& _duration) : errorCode("errorCode")
{
  subscriptionId.set(_subscriptionId);
  duration.set(_duration);
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse -
*/
SubscribeContextAvailabilityResponse::SubscribeContextAvailabilityResponse(const std::string& _subscriptionId, StatusCode& _errorCode) : errorCode("errorCode")
{
  subscriptionId.set(_subscriptionId);

  errorCode.fill(&_errorCode);
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityResponse::render -
*/
std::string SubscribeContextAvailabilityResponse::render(void)
{
  std::string  out                = "";
  bool         durationRendered   = !duration.isEmpty();
  bool         errorCodeRendered  = (errorCode.code != SccNone);

  out += startTag();

  out += subscriptionId.render(RtSubscribeContextAvailabilityResponse, durationRendered || errorCodeRendered);
  out += duration.render(errorCodeRendered);

  if (errorCodeRendered)
     out += errorCode.render(false);

  out += endTag();
  
  return out;
}
