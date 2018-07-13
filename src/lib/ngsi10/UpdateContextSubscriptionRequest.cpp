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

  if (predetectedError != "")
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

  return response.render();
}



/* ****************************************************************************
*
* UpdateContextSubscriptionRequest::release - 
*/
void UpdateContextSubscriptionRequest::release(void)
{
  // Old versions of this method also include a 'restriction.release()' call. However, now each time
  // a UpdateContextSubscriptionRequest is created, the method toNgsiv2Subscription() is used on it and the
  // 'ownership' of the Restriction is transferred to the corresponding NGSIv2 class. Thus, leaving
  // that 'restriction.release()' would cause double-free problems

  notifyConditionVector.release();
}



/* ****************************************************************************
*
* UpdateContextSubscriptionRequest::toNgsiv2Subscription -
*/
void UpdateContextSubscriptionRequest::toNgsiv2Subscription(SubscriptionUpdate* subUp)
{
  // Parent method will do most of the work
  SubscribeContextRequest::toNgsiv2Subscription(subUp);

  // Fill remaining fields in SubscriptionUpdate
  subUp->id         = subscriptionId.get();
  subUp->fromNgsiv1 = true;

  // Fields that can be modified in a NGSIv1 subscription
  // (See https://fiware-orion.readthedocs.io/en/master/user/updating_regs_and_subs/index.html)
  //
  //  * notifyConditions (within subject in NGSIv2)
  //  * throttling       (root field in NGSIv2)
  //  * duration         (root field -as 'expires'- in NGSIv2)
  //  * restriction      (already processed in the parent method)

  subUp->subjectProvided      = (notifyConditionVector.size() > 0);
  subUp->expiresProvided      = !duration.isEmpty();
  subUp->statusProvided       = false;  // not supported in NGSIv1
  subUp->notificationProvided = false;  // NGSIv1 doesn's allow changes in that part
  subUp->attrsFormatProvided  = true;   // updating in NGSIv1 involves and implicit change to NGSIv1 legacy format
  subUp->throttlingProvided   = !throttling.isEmpty();

}
