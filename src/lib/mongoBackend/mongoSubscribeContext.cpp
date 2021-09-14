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
#include "ngsi/Duration.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "apiTypesV2/Subscription.h"

#include "mongoBackend/mongoCreateSubscription.h"
#include "mongoBackend/mongoSubscribeContext.h"



/* ****************************************************************************
*
* mongoSubscribeContext - 
*/
HttpStatusCode mongoSubscribeContext
(
  SubscribeContextRequest*             requestP,
  SubscribeContextResponse*            responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV
)
{
  OrionError            oe;
  ngsiv2::Subscription  sub;

  requestP->toNgsiv2Subscription(&sub);
  std::string subId = mongoCreateSubscription(sub, &oe, tenant, servicePathV);

  if (!subId.empty())
  {
    if (requestP->duration.isEmpty())
    {
      responseP->subscribeResponse.duration.set(DEFAULT_DURATION);
    }
    else
    {
      responseP->subscribeResponse.duration = requestP->duration;
    }
    responseP->subscribeResponse.subscriptionId.set(subId);
    responseP->subscribeResponse.throttling = requestP->throttling;
  }
  else
  {
    // Check OrionError
    responseP->subscribeError.errorCode.fill(oe.code, oe.details);
  }

  return SccOk;
}
