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
*/
typedef enum RequestType
{
  NoRequest,
  RegisterContext = 1,
  DiscoverContextAvailability,
  SubscribeContextAvailability,
  RtSubscribeContextAvailabilityResponse,
  UpdateContextAvailabilitySubscription,
  RtUpdateContextAvailabilitySubscriptionResponse,
  UnsubscribeContextAvailability,
  RtUnsubscribeContextAvailabilityResponse,
  NotifyContextAvailability,

  QueryContext = 11,
  RtQueryContextResponse,
  SubscribeContext,
  UpdateContextSubscription,
  UnsubscribeContext,
  RtUnsubscribeContextResponse,
  NotifyContext,
  UpdateContext,
  RtUpdateContextResponse,
  NotifyContextSent,

  ContextEntitiesByEntityId = 21,
  ContextEntityAttributes,
  EntityByIdAttributeByName,
  ContextEntityTypes,
  ContextEntityTypeAttributeContainer,
  ContextEntityTypeAttribute,
  Ngsi9SubscriptionsConvOp,

  IndividualContextEntity                = 31,
  IndividualContextEntityAttributes,
  IndividualContextEntityAttribute,
  IndividualContextEntityAttributeWithTypeAndId,
  AttributeValueInstance,
  AttributeValueInstanceWithTypeAndId,
  Ngsi10ContextEntityTypes,
  Ngsi10ContextEntityTypesAttributeContainer,
  Ngsi10ContextEntityTypesAttribute,
  Ngsi10SubscriptionsConvOp,

  UpdateContextElement = 41,
  AppendContextElement,
  UpdateContextAttribute,

  LogTraceRequest = 51,
  LogLevelRequest,
  SemStateRequest,
  MetricsRequest,
  VersionRequest,
  ExitRequest,

  LeakRequest,
  StatisticsRequest,
  RegisterResponse,
  RtSubscribeResponse,
  RtSubscribeError,

  RtContextElementResponse,
  RtContextAttributeResponse,

  EntityTypes = 65,
  AttributesForEntityType,
  RtEntityTypesResponse,
  RtAttributesForEntityTypeResponse,
  AllContextEntities,
  AllEntitiesWithTypeAndId,
  ContextEntitiesByEntityIdAndType,
  EntityByIdAttributeByNameIdAndType,

  // /v2 API
  EntitiesRequest = 75,
  EntitiesResponse,
  EntryPointsRequest,
  EntryPointsResponse,
  EntityRequest,
  EntityResponse,
  EntityAttributeRequest,
  EntityAttributeResponse,
  EntityAttributeValueRequest,
  EntityAttributeValueResponse,
  PostEntity,
  PostAttributes,
  DeleteEntity,
  EntityTypeRequest,
  EntityAllTypesRequest,
  SubscriptionsRequest,
  IndividualSubscriptionRequest,
  BatchQueryRequest,
  BatchUpdateRequest,

  // v2 registration
  RegistrationRequest,
  RegistrationsRequest,

  InvalidRequest = 100,

  // PoC
  getNgsiTestRequest = 200

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

#endif  // SRC_LIB_NGSI_REQUEST_H_
