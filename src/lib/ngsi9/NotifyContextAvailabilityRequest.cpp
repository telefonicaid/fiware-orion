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
* Author: Fermin Galan
*/
#include <string>

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi9/NotifyContextAvailabilityRequest.h"
#include "ngsi9/NotifyContextAvailabilityResponse.h"



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::NotifyContextAvailabilityRequest - 
*/
NotifyContextAvailabilityRequest::NotifyContextAvailabilityRequest()
{
}



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::toJsonV1 -
*/
std::string NotifyContextAvailabilityRequest::toJsonV1(void)
{
  std::string out = "";

  //
  // Note on JSON commas:
  //  Both subscriptionId and contextRegistrationResponseVector are MANDATORY.
  //  Always comma for subscriptionId.
  //  With an empty contextRegistrationResponseVector there would be no notification
  //
  out += startTag();
  out += subscriptionId.toJsonV1(NotifyContextAvailability, true);
  out += contextRegistrationResponseVector.toJsonV1(false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::check - 
*/
std::string NotifyContextAvailabilityRequest::check(ApiVersion apiVersion, const std::string& predetectedError)
{
  std::string                        res;
  NotifyContextAvailabilityResponse  response;

  if (!predetectedError.empty())
  {
    response.responseCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = subscriptionId.check())                                                                 != "OK") ||
           ((res = contextRegistrationResponseVector.check(apiVersion, QueryContext, predetectedError, 0)) != "OK"))
  {
    response.responseCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.toJsonV1();
}


/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::release -
*/
void NotifyContextAvailabilityRequest::release(void)
{
  contextRegistrationResponseVector.release();
}
