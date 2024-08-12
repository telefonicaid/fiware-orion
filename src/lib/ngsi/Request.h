#ifndef SRC_LIB_NGSI_REQUEST_H_
#define SRC_LIB_NGSI_REQUEST_H_

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

struct ConnectionInfo;



/* ****************************************************************************
*
* RequestType - 
*
* FIXME PR: review this
*/
typedef enum RequestType
{
  NoRequest = 0,

  // pure v2
  EntryPointsRequest,
  EntitiesRequest,  
  EntityRequest,
  EntityAttributeRequest,
  EntityAttributeValueRequest,
  EntityAllTypesRequest,
  EntityTypeRequest,
  SubscriptionsRequest,
  SubscriptionRequest,
  RegistrationsRequest,
  RegistrationRequest,
  BatchQueryRequest,
  BatchUpdateRequest,
  NotifyContext,

  // administrative requests
  LogTraceRequest,
  StatisticsRequest,
  LogLevelRequest,
  SemStateRequest,
  VersionRequest,
  MetricsRequest,

  // requests enabled in DEBUG compilation
  ExitRequest,
  LeakRequest,

  InvalidRequest
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
* requestTypeForCounter -
*/
extern std::string requestTypeForCounter(RequestType rt, const std::string& prefix);



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

#endif  // SRC_LIB_NGSI_REQUEST_H_
