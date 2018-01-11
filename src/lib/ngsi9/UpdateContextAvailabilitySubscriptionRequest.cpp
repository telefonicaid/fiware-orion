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

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionRequest.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"



/* ****************************************************************************
*
* Constructor -
*/
UpdateContextAvailabilitySubscriptionRequest::UpdateContextAvailabilitySubscriptionRequest()
{
   restrictions = 0;
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::render -
*/
std::string UpdateContextAvailabilitySubscriptionRequest::render(void)
{  
  std::string   out                      = "";
  bool          subscriptionRendered     = subscriptionId.rendered(UpdateContextAvailabilitySubscription);
  bool          restrictionRendered      = restrictions != 0;
  bool          durationRendered         = duration.get() != "";
  bool          attributeListRendered    = attributeList.size() != 0;
  bool          commaAfterSubscriptionId = false; // last element
  bool          commaAfterRestriction    = subscriptionRendered;
  bool          commaAfterDuration       = restrictionRendered || subscriptionRendered;
  bool          commaAfterAttributeList  = durationRendered || restrictionRendered || subscriptionRendered;
  bool          commaAfterEntityIdVector = attributeListRendered || durationRendered || restrictionRendered || subscriptionRendered;

  out += startTag();
  out += entityIdVector.render(commaAfterEntityIdVector);
  out += attributeList.render(commaAfterAttributeList, "attributes");
  out += duration.render(commaAfterDuration);
  out += restriction.render(restrictions, commaAfterRestriction);
  out += subscriptionId.render(UpdateContextAvailabilitySubscription, commaAfterSubscriptionId);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::present -
*/
void UpdateContextAvailabilitySubscriptionRequest::present(const std::string& indent)
{
   entityIdVector.present(indent + "  ");
   attributeList.present(indent + "  ");
   duration.present(indent + "  ");
   restriction.present(indent + "  ");
   subscriptionId.present(indent + "  ");
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::check -
*/
std::string UpdateContextAvailabilitySubscriptionRequest::check(const std::string& predetectedError, int counter)
{
  std::string                                    res;
  UpdateContextAvailabilitySubscriptionResponse  response;

  response.subscriptionId = subscriptionId;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(UpdateContextAvailabilitySubscription)) != "OK") ||
           ((res = attributeList.check())                                       != "OK") ||
           ((res = duration.check())                                            != "OK") ||
           ((res = restriction.check(counter))                                  != "OK") ||
           ((res = subscriptionId.check())                                      != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render();
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::release -
*/
void UpdateContextAvailabilitySubscriptionRequest::release(void)
{
  entityIdVector.release();
  attributeList.release();
  duration.release();
  restriction.release();
  subscriptionId.release();
}
