#ifndef SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONREQUEST_H_
#define SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONREQUEST_H_

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

#include "ngsi/Request.h"
#include "ngsi/EntityId.h"
#include "ngsi/Duration.h"
#include "ngsi/NotifyCondition.h"
#include "ngsi/Restriction.h"
#include "ngsi/SubscriptionId.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/Throttling.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "apiTypesV2/SubscriptionUpdate.h"


/* ****************************************************************************
*
* UpdateContextSubscriptionRequest -
*/
struct UpdateContextSubscriptionRequest : public SubscribeContextRequest
{
  SubscriptionId                 subscriptionId;         // Mandatory

  UpdateContextSubscriptionRequest();  
  std::string check(const std::string& predetectedError, int counter);
  void        release(void);
  void        toNgsiv2Subscription(ngsiv2::SubscriptionUpdate* subUp);
};

#endif  // SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONREQUEST_H_
