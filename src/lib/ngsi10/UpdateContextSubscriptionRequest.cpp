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
#include <stdio.h>
#include <string>

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/SubscribeContextRequest.h"

using namespace ngsiv2;



/* ****************************************************************************
*
* UpdateContextSubscriptionRequest::UpdateContextSubscriptionRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
UpdateContextSubscriptionRequest::UpdateContextSubscriptionRequest()
{
  restrictions = 0;
}



/* ****************************************************************************
*
* UpdateContextSubscriptionRequest::check - 
*/
std::string UpdateContextSubscriptionRequest::check(const std::string& predetectedError, int counter)
{
  std::string                       res;
  UpdateContextSubscriptionResponse response;

  if (!predetectedError.empty())
  {
    response.subscribeError.subscriptionId = subscriptionId;
    response.subscribeError.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = duration.check())                                                                  != "OK") ||
           ((res = restriction.check(restrictions))                                                   != "OK") ||
           ((res = subscriptionId.check())                                                            != "OK") ||
           ((res = notifyConditionVector.check(UpdateContextSubscription, predetectedError, counter)) != "OK") ||
           ((res = throttling.check())                                                                != "OK"))
  {
    response.subscribeError.subscriptionId = subscriptionId;
    response.subscribeError.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.toJsonV1();
}



/* ****************************************************************************
*
* UpdateContextSubscriptionRequest::release - 
*/
void UpdateContextSubscriptionRequest::release(void)
{
  restriction.release();
  notifyConditionVector.release();
}