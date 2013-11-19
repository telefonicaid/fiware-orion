#ifndef SUBSCRIBE_CONTEXT_AVAILABILITY_REQUEST_H
#define SUBSCRIBE_CONTEXT_AVAILABILITY_REQUEST_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "common/Format.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Reference.h"
#include "ngsi/Restriction.h"
#include "ngsi/SubscriptionId.h"



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest - 
*/
typedef struct SubscribeContextAvailabilityRequest
{
  EntityIdVector         entityIdVector;    // Mandatory
  AttributeList          attributeList;     // Optional
  Reference              reference;         // Mandatory
  Duration               duration;          // Optional
  Restriction            restriction;       // Optional
  SubscriptionId         subscriptionId;    // Optional

  /* The number of restrictions */
  int                    restrictions;

  SubscribeContextAvailabilityRequest();
  std::string  render(RequestType requestType, Format format, std::string indent);
  std::string  check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void         release(void);
  void         present(std::string indent);
} SubscribeContextAvailabilityRequest;

#endif
