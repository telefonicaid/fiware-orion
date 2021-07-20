#ifndef SRC_LIB_NGSI10_SUBSCRIBECONTEXTREQUEST_H_
#define SRC_LIB_NGSI10_SUBSCRIBECONTEXTREQUEST_H_

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

#include "common/RenderFormat.h"
#include "ngsi/Request.h"
#include "ngsi/StringList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/Reference.h"
#include "ngsi/MaxFailsLimit.h"
#include "ngsi/Restriction.h"
#include "ngsi/Throttling.h"
#include "rest/EntityTypeInfo.h"
#include "apiTypesV2//Subscription.h"



/* ****************************************************************************
*
* SubscribeContextRequest - 
*/
typedef struct SubscribeContextRequest
{
  EntityIdVector         entityIdVector;         // Mandatory
  StringList             attributeList;          // Optional
  Reference              reference;              // Mandatory
  Duration               duration;               // Optional
  Restriction            restriction;            // Optional
  NotifyConditionVector  notifyConditionVector;  // Optional
  Throttling             throttling;             // Optional
  MaxFailsLimit          maxFailsLimit;          // Optional

  /* The number of restrictions */
  int                    restrictions;

  SubscribeContextRequest(): restrictions(0) {}

  std::string  check(const std::string& predetectedError, int counter);
  void         release(void);
  void         toNgsiv2Subscription(ngsiv2::Subscription* sub);

  void         fill(EntityTypeInfo typeInfo);
} SubscribeContextRequest;

#endif  // SRC_LIB_NGSI10_SUBSCRIBECONTEXTREQUEST_H_
