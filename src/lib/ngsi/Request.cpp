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
#include "ngsi/Request.h"



/* ****************************************************************************
*
* requestType -
*/
const char* requestType(RequestType rt)
{
  switch (rt)
  {
  case NoRequest:                                   return "NoRequest";
  case RegisterContext:                             return "RegisterContextRequest";
  case RegisterResponse:                            return "RegisterContextResponse";
  case DiscoverContextAvailability:                 return "DiscoverContextAvailabilityRequest";   
  case QueryContext:                                return "QueryContextRequest";
  case RtQueryContextResponse:                      return "QueryContextResponse";
  case SubscribeContext:                            return "SubscribeContextRequest";
  case UpdateContextSubscription:                   return "UpdateContextSubscriptionRequest";
  case UnsubscribeContext:                          return "UnsubscribeContextRequest";
  case NotifyContext:                               return "NotifyContextRequest";
  case NotifyContextSent:                           return "NotifyContextRequestSent";
  case UpdateContext:                               return "UpdateContextRequest";
  case RtUpdateContextResponse:                     return "UpdateContextResponse";

  case ContextEntitiesByEntityId:                   return "ContextEntitiesByEntityId";
  case ContextEntityAttributes:                     return "ContextEntityAttributes";
  case EntityByIdAttributeByName:                   return "EntityByIdAttributeByName";
  case ContextEntityTypes:                          return "ContextEntityTypes";
  case ContextEntityTypeAttributeContainer:         return "ContextEntityTypeAttributeContainer";
  case ContextEntityTypeAttribute:                  return "ContextEntityTypeAttribute";

  case IndividualContextEntity:                     return "IndividualContextEntity";
  case IndividualContextEntityAttributes:           return "IndividualContextEntityAttributes";
  case AttributeValueInstance:                      return "AttributeValueInstance";
  case AttributeValueInstanceWithTypeAndId:         return "AttributeValueInstanceWithTypeAndId";
  case IndividualContextEntityAttribute:            return "IndividualContextEntityAttribute";
  case IndividualContextEntityAttributeWithTypeAndId:  return "IndividualContextEntityAttributeWithTypeAndId";
  case UpdateContextElement:                        return "UpdateContextElement";
  case AppendContextElement:                        return "AppendContextElement";
  case UpdateContextAttribute:                      return "UpdateContextAttribute";
  case Ngsi10ContextEntityTypes:                    return "Ngsi10ContextEntityTypes";
  case Ngsi10ContextEntityTypesAttributeContainer:  return "Ngsi10ContextEntityTypesAttributeContainer";
  case Ngsi10ContextEntityTypesAttribute:           return "Ngsi10ContextEntityTypesAttribute";
  case Ngsi10SubscriptionsConvOp:                   return "Ngsi10SubscriptionsConvOp";

  case LogTraceRequest:                             return "LogTrace";
  case LogLevelRequest:                             return "LogLevel";
  case SemStateRequest:                             return "SemState";
  case MetricsRequest:                              return "Metrics";
  case VersionRequest:                              return "Version";
  case StatisticsRequest:                           return "Statistics";
  case ExitRequest:                                 return "Exit";
  case LeakRequest:                                 return "Leak";
  case InvalidRequest:                              return "InvalidRequest";

  case RtUnsubscribeContextResponse:                     return "UnsubscribeContextResponse";
  case RtSubscribeResponse:                              return "SubscribeResponse";
  case RtSubscribeError:                                 return "SubscribeError";
  case RtContextElementResponse:                         return "ContextElementResponse";
  case RtContextAttributeResponse:                       return "ContextAttributeResponse";
  case RtEntityTypesResponse:                            return "EntityTypesResponse";
  case RtAttributesForEntityTypeResponse:                return "AttributesForEntityTypeResponse";
  case EntityTypes:                                      return "EntityTypes";
  case AttributesForEntityType:                          return "AttributesForEntityType";
  case AllContextEntities:                               return "AllContextEntities";
  case AllEntitiesWithTypeAndId:                         return "AllEntitiesWithTypeAndId";
  case ContextEntitiesByEntityIdAndType:                 return "ContextEntitiesByEntityIdAndType";
  case EntityByIdAttributeByNameIdAndType:               return "EntityByIdAttributeByNameIdAndType";

  case EntitiesRequest:                                  return "EntitiesRequest";
  case EntitiesResponse:                                 return "EntitiesResponse";

  case EntryPointsRequest:                               return "EntryPointsRequest";
  case EntryPointsResponse:                              return "EntryPointsResponse";

  case EntityRequest:                                    return "EntityRequest";
  case EntityResponse:                                   return "EntityResponse";
  case EntityAttributeRequest:                           return "EntityAttributeRequest";
  case EntityAttributeResponse:                          return "EntityAttributeResponse";
  case EntityAttributeValueRequest:                      return "EntityAttributeValueRequest";
  case EntityAttributeValueResponse:                     return "EntityAttributeValueResponse";
  case PostEntity:                                       return "PostEntity";
  case PostAttributes:                                   return "PostAttributes";
  case DeleteEntity:                                     return "DeleteEntity";

  case EntityTypeRequest:                                return "EntityTypeRequest";
  case EntityAllTypesRequest:                            return "EntityAllTypesRequest";
  case SubscriptionsRequest:                             return "SubscriptionsRequest";
  case IndividualSubscriptionRequest:                    return "IndividualSubscriptionRequest";
  case BatchQueryRequest:                                return "BatchQueryRequest";
  case BatchUpdateRequest:                               return "BatchUpdateRequest";

  case RegistrationRequest:                              return "RegistrationRequest";
  case RegistrationsRequest:                             return "RegistrationsRequest";
  }

  return "";
}
