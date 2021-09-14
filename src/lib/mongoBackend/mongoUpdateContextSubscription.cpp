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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <vector>

#include "rest/OrionError.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"
#include "apiTypesV2/SubscriptionUpdate.h"

#include "mongoBackend/mongoUpdateSubscription.h"
#include "mongoBackend/mongoUpdateContextSubscription.h"



/* ****************************************************************************
*
* mongoUpdateContextSubscription -
*/
HttpStatusCode mongoUpdateContextSubscription
(
  UpdateContextSubscriptionRequest*   requestP,
  UpdateContextSubscriptionResponse*  responseP,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV
)
{
  OrionError                  oe;
  ngsiv2::SubscriptionUpdate  sub;

  requestP->toNgsiv2Subscription(&sub);

  std::string subId = mongoUpdateSubscription(sub, &oe, tenant, servicePathV);

  if (!subId.empty())
  {
    // Duration and throttling are optional parameters, they are only added in the case they were used for update
    if (!requestP->duration.isEmpty())
    {
      responseP->subscribeResponse.duration = requestP->duration;
    }

    if (!requestP->throttling.isEmpty())
    {
      responseP->subscribeResponse.throttling = requestP->throttling;
    }

    responseP->subscribeResponse.subscriptionId = subId;
  }
  else
  {
    //
    // Check OrionError. Depending on the kind of error, details are included or not
    // in order to have a better backward compatiblity
    //
    // FIXME: should we? or it is better to modify .test and provide more accurate errors with 'details'?
    //
    if (oe.code == SccContextElementNotFound)
    {
      responseP->subscribeError.errorCode.fill(oe.code);
    }
    else
    {
      responseP->subscribeError.errorCode.fill(oe.code, oe.details);
    }
  }

  return SccOk;
}
