#ifndef REQUEST_H
#define REQUEST_H

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

#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* RequestType - 
*/
typedef enum RequestType
{
  RegisterContext = 0,
  DiscoverContextAvailability,
  SubscribeContextAvailability,
  UpdateContextAvailabilitySubscription,
  UnsubscribeContextAvailability,
  NotifyContextAvailability,

  QueryContext = 6,
  SubscribeContext,
  UpdateContextSubscription,
  UnsubscribeContext,
  NotifyContext,
  UpdateContext,

  ContextEntitiesByEntityId = 12,
  ContextEntityAttributes,
  EntityByIdAttributeByName,

  IndividualContextEntity                = 15,
  IndividualContextEntityAttributes,
  IndividualContextEntityAttribute,

  UpdateContextElement = 18,
  AppendContextElement,
  UpdateContextAttribute,

  LogRequest = 21,
  VersionRequest,
  ExitRequest,
  LeakRequest,
  StatisticsRequest,

  InvalidRequest,
  RegisterResponse
} RequestType;



/* ****************************************************************************
*
* Forward declarations
*/
struct ParseData;



/* ****************************************************************************
*
* requestType - 
*/
extern const char* requestType(RequestType rt);



/* ****************************************************************************
*
* RequestInit - 
*/
typedef void (*RequestInit)(ParseData* reqDataP);



/* ****************************************************************************
*
* RequestRelease - 
*/
typedef void (*RequestRelease)(ParseData* reqDataP);



/* ****************************************************************************
*
* RequestCheck - 
*/
typedef std::string (*RequestCheck)(ParseData* reqDataP, ConnectionInfo* ciP);



/* ****************************************************************************
*
* RequestPresent - 
*/
typedef void (*RequestPresent)(ParseData* reqDataP);

#endif
