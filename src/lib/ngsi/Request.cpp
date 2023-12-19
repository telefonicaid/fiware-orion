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
#include <algorithm>

#include "ngsi/Request.h"

#include "logMsg/logMsg.h"



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

  case EntityTypeRequest:                                return "EntityTypeRequest";
  case EntityAllTypesRequest:                            return "EntityAllTypesRequest";
  case SubscriptionsRequest:                             return "SubscriptionsRequest";
  case SubscriptionRequest:                              return "SubscriptionRequest";
  case BatchQueryRequest:                                return "BatchQueryRequest";
  case BatchUpdateRequest:                               return "BatchUpdateRequest";

  case RegistrationRequest:                              return "RegistrationRequest";
  case RegistrationsRequest:                             return "RegistrationsRequest";
  }

  return "";
}



/* ****************************************************************************
*
* requestTypeForCounter -
*/
std::string requestTypeForCounter(RequestType rt, const std::string& _prefix)
{
  // To lowercase
  std::string prefix = "/" + _prefix;
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

  switch (rt)
  {
  // pure v2
  case EntryPointsRequest:             return "/v2";
  case EntitiesRequest:                return "/v2/entities";
  case EntityRequest:                  return "/v2/entities/{id}[/attrs]";
  case EntityAttributeRequest:         return "/v2/entities/{id}/attrs/{name}";
  case EntityAttributeValueRequest:    return "/v2/entities/{id}/attrs/{name}/value";
  case EntityAllTypesRequest:          return "/v2/types";
  case EntityTypeRequest:              return "/v2/types/{type}";
  case SubscriptionsRequest:           return "/v2/subscriptions";
  case SubscriptionRequest:            return "/v2/subscriptions/{id}";
  case RegistrationsRequest:           return "/v2/registrations";
  case RegistrationRequest:            return "/v2/registrations/{id}";
  case BatchQueryRequest:              return "/v2/op/query";
  case BatchUpdateRequest:             return "/v2/op/update";

  // administrative requests
  case LogTraceRequest:                return "/log/trace[/{level}]";
  case LogLevelRequest:                return "/admin/log";
  case SemStateRequest:                return "/admin/sem";
  case MetricsRequest:                 return "/admin/metrics";
  case ExitRequest:                    return "/exit[/*]";
  case LeakRequest:                    return "/leak[/*]";
  case VersionRequest:                 return "/version";
  case StatisticsRequest:
    if (prefix == "/cache")
    {
      return "/cache/statistics";
    }
    else
    {
      return "/statistics";
    }

  // pure v1
  // FIXME: disable unused NGSv1 API routes in Orion 3.9.0, to be definetively removed at some point of the future
  //case RegisterContext:                      return "/v1/registry/registerContext";
  //case ContextEntitiesByEntityId:            return "/v1/registry/contextEntities/{id}";
  //case ContextEntitiesByEntityIdAndType:     return "/v1/registry/contextEntities/type/{type}/id/{id}";
  //case ContextEntityAttributes:              return "/v1/registry/contextEntities/{id}/attributes";
  //case ContextEntityTypeAttribute:           return "/v1/registry/contextEntityTypes/{type}/attributes/{name}";
  //case ContextEntityTypeAttributeContainer:  return "/v1/registry/contextEntityTypes/{type}/attributes";
  //case ContextEntityTypes:                   return "/v1/registry/contextEntityTypes/{type}";
  //case DiscoverContextAvailability:          return "/v1/registry/discoverContextAvailability";
  //case EntityByIdAttributeByName:            return "/v1/registry/contextEntities/{id}/attributes/{name}";
  //case EntityByIdAttributeByNameIdAndType:   return "/v1/registry/contextEntities/type/{type}/id/{id}/attributes/{name}";
  //case EntityTypes:                          return "/v1/contextTypes";
  case RegisterContext:
  case ContextEntitiesByEntityId:
  case ContextEntitiesByEntityIdAndType:
  case ContextEntityAttributes:
  case ContextEntityTypeAttribute:
  case ContextEntityTypeAttributeContainer:
  case ContextEntityTypes:
  case DiscoverContextAvailability:
  case EntityByIdAttributeByName:
  case EntityByIdAttributeByNameIdAndType:
  case EntityTypes:
    return "skip";

  // v1 or NGSI10;
  // FIXME: disable unused NGSv1 API routes in Orion 3.9.0, to be definetively removed at some point of the future
  //case AllContextEntities:                             return prefix + "/contextEntitites";
  //case AllEntitiesWithTypeAndId:                       return prefix + "/contextEntities/type/{type}/id/{id}";
  //case AttributesForEntityType:                        return prefix + "/contextType/{type}";
  //case IndividualContextEntity:                        return prefix + "/contextEntities/{id}";
  case IndividualContextEntity:
    if (prefix == "/v1")
    {
      return "/v1/contextEntities/{id}";
    }
    else
    {
      // ngsi10 case
      return "skip";
    }

  //case IndividualContextEntityAttribute:               return prefix + "/contextEntities/{id}/attributes/{name}";
  case IndividualContextEntityAttribute:
    if (prefix == "/v1")
    {
      return "/v1/contextEntities/{id}/attributes/{name}";
    }
    else
    {
      // ngsi10 case
      return "skip";
    }
  //case IndividualContextEntityAttributes:              return prefix + "/contextEntities/{id}/attributes/";
  //case IndividualContextEntityAttributeWithTypeAndId:  return prefix + "/contextEntities/type/{type}/id/{id}/attributes/{name}";
  //case Ngsi10ContextEntityTypes:                       return prefix + "/contextEntityTypes/{type}";
  //case Ngsi10ContextEntityTypesAttribute:              return prefix + "/contextEntityTypes/{type}/attributes/{name}";
  //case Ngsi10ContextEntityTypesAttributeContainer:     return prefix + "/contextEntityTypes/{type}/attributes/";
  //case Ngsi10SubscriptionsConvOp:                      return prefix + "/contextSubscriptions/{id}";
  case QueryContext:                                   return prefix + "/queryContext";
  //case SubscribeContext:                               return prefix + "/subscribeContext|contextSubscriptions";
  //case UnsubscribeContext:                             return prefix + "/unsubscribeContext";
  case UpdateContext:                                  return prefix + "/updateContext";
  //case UpdateContextSubscription:                      return prefix + "/updateContextSubscription";
  case AllContextEntities:
  case AllEntitiesWithTypeAndId:
  case AttributesForEntityType:
  //case IndividualContextEntity:
  //case IndividualContextEntityAttribute:
  case IndividualContextEntityAttributes:
  case IndividualContextEntityAttributeWithTypeAndId:
  case Ngsi10ContextEntityTypes:
  case Ngsi10ContextEntityTypesAttribute:
  case Ngsi10ContextEntityTypesAttributeContainer:
  case Ngsi10SubscriptionsConvOp:
  //case QueryContext:
  case SubscribeContext:
  case UnsubscribeContext:
  //case UpdateContext:
  case UpdateContextSubscription:
    return "skip";

  // v2, v1 or NGSIv2
  case NotifyContext:
    if (prefix == "/v2")
    {
      return "/v2/op/notify";
    }
    else  // v1 or NGSI10 case
    {
      // FIXME: disable unused NGSv1 API routes in Orion 3.9.0, to be definetively removed at some point of the future
      //return prefix + "/notifyContext";
      return "skip";
    }

  default:
    LM_E(("Runtime Error (unclasified request %d, %s, prefix %s", rt, requestType(rt), prefix.c_str()));
    return "/unclasified";
  }
}
