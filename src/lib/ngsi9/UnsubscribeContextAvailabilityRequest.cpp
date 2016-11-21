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

#include "ngsi/SubscriptionId.h"
#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityRequest::UnsubscribeContextAvailabilityRequest - 
*/
UnsubscribeContextAvailabilityRequest::UnsubscribeContextAvailabilityRequest()
{
   subscriptionId.set("");
}



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityRequest::UnsubscribeContextAvailabilityRequest - 
*/
UnsubscribeContextAvailabilityRequest::UnsubscribeContextAvailabilityRequest(SubscriptionId& _subscriptionId)
{
   subscriptionId.set(_subscriptionId.get());
}



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityRequest::check - 
*/
std::string UnsubscribeContextAvailabilityRequest::check(const std::string& indent, const std::string& predetectedError, int counter)
{
   UnsubscribeContextAvailabilityResponse  response(subscriptionId);
   std::string                             res;

  if (predetectedError != "")
  {
    response.statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = subscriptionId.check(UnsubscribeContextAvailability, indent, predetectedError, counter)) != "OK")
  {
    response.statusCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render(indent);
}



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityRequest::release - 
*/
void UnsubscribeContextAvailabilityRequest::release(void)
{
   subscriptionId.release();
}



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityRequest::fill - 
*/
void UnsubscribeContextAvailabilityRequest::fill(const std::string& _subscriptionId)
{
  subscriptionId.set(_subscriptionId);
}
