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
#include <vector>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionResponse::UpdateContextAvailabilitySubscriptionResponse - 
*/
UpdateContextAvailabilitySubscriptionResponse::UpdateContextAvailabilitySubscriptionResponse()
{
  errorCode.keyNameSet("errorCode");
}

/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionResponse::UpdateContextAvailabilitySubscriptionResponse - 
*/
UpdateContextAvailabilitySubscriptionResponse::UpdateContextAvailabilitySubscriptionResponse(StatusCode& _errorCode)
{
  errorCode.fill(&_errorCode);
  errorCode.keyNameSet("errorCode");
}

/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionResponse::~UpdateContextAvailabilitySubscriptionResponse -
*/
UpdateContextAvailabilitySubscriptionResponse::~UpdateContextAvailabilitySubscriptionResponse()
{
   subscriptionId.release();
   duration.release();
   errorCode.release();

   LM_T(LmtDestructor,("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionResponse::toJsonV1 -
*/
std::string UpdateContextAvailabilitySubscriptionResponse::toJsonV1(void)
{
  std::string  out                = "";
  bool         durationRendered   = !duration.isEmpty();
  bool         errorCodeRendered  = (errorCode.code != SccNone);

  out += startTag();

  out += subscriptionId.toJsonV1(RtUpdateContextAvailabilitySubscriptionResponse, errorCodeRendered || durationRendered);
  out += duration.toJsonV1(errorCodeRendered);

  if (errorCodeRendered)
  {
    out += errorCode.toJsonV1(false);
  }

  out += endTag();

  return out;
}

/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionResponse::check - 
*/
std::string UpdateContextAvailabilitySubscriptionResponse::check(const std::string& predetectedError)
{
  std::string  res;

  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = subscriptionId.check()) != "OK") ||
           ((res = duration.check())       != "OK"))
  {
    errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return toJsonV1();
}
