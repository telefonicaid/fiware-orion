#ifndef SRC_LIB_NGSI9_NOTIFYCONTEXTAVAILABILITYREQUEST_H_
#define SRC_LIB_NGSI9_NOTIFYCONTEXTAVAILABILITYREQUEST_H_

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

#include "ngsi/Request.h"
#include "ngsi/SubscriptionId.h"
#include "ngsi/ContextRegistrationResponseVector.h"


/* ****************************************************************************
*
* NotifyContextAvailabilityRequest -
*/
typedef struct NotifyContextAvailabilityRequest
{
  SubscriptionId                     subscriptionId;                     // Mandatory
  ContextRegistrationResponseVector  contextRegistrationResponseVector;  // Mandatory

  NotifyContextAvailabilityRequest();

  std::string   render(void);
  std::string   check(ApiVersion apiVersion, const std::string& predetectedError);
  void          present(const std::string& indent);
  void          release(void);
} NotifyContextAvailabilityRequest;

#endif  // SRC_LIB_NGSI9_NOTIFYCONTEXTAVAILABILITYREQUEST_H_
