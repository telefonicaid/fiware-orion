#ifndef SRC_LIB_NGSI9_UPDATECONTEXTAVAILABILITYSUBSCRIPTIONREQUEST_H_
#define SRC_LIB_NGSI9_UPDATECONTEXTAVAILABILITYSUBSCRIPTIONREQUEST_H_

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

#include "ngsi/Request.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Restriction.h"
#include "ngsi/SubscriptionId.h"



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest - 
*/
typedef struct UpdateContextAvailabilitySubscriptionRequest
{
  EntityIdVector  entityIdVector;    // Mandatory
  AttributeList   attributeList;     // Optional
  Duration        duration;          // Optional
  Restriction     restriction;       // Optional
  SubscriptionId  subscriptionId;    // Mandatory (error in OMA spec?  OMA spec says it's optional ...)

  /* The number of restrictions */
  int                    restrictions;

  UpdateContextAvailabilitySubscriptionRequest();

  std::string     render(const std::string& indent);
  void            present(const std::string& indent);
  std::string     check(const std::string& indent, const std::string& predetectedError, int counter);
  void            release(void);
} UpdateContextAvailabilitySubscriptionRequest;

#endif  // SRC_LIB_NGSI9_UPDATECONTEXTAVAILABILITYSUBSCRIPTIONREQUEST_H_
