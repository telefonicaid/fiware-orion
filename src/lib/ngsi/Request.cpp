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
*
*/
const char* requestType(RequestType rt)
{
  switch (rt)
  {
  case NoRequest:                                   return "NoRequest";

  // pure v2
  case EntryPointsRequest:                          return "EntryPointsRequest";
  case EntitiesRequest:                             return "EntitiesRequest";
  case EntityRequest:                               return "EntityRequest";
  case EntityAttributeRequest:                      return "EntityAttributeRequest";
  case EntityAttributeValueRequest:                 return "EntityAttributeValueRequest";
  case EntityAllTypesRequest:                       return "EntityAllTypesRequest";
  case EntityTypeRequest:                           return "EntityTypeRequest";
  case SubscriptionsRequest:                        return "SubscriptionsRequest";
  case SubscriptionRequest:                         return "SubscriptionRequest";
  case RegistrationRequest:                         return "RegistrationRequest";
  case RegistrationsRequest:                        return "RegistrationsRequest";
  case BatchQueryRequest:                           return "BatchQueryRequest";
  case BatchUpdateRequest:                          return "BatchUpdateRequest";
  case NotifyContext:                               return "NotifyContextRequest";

  // administrative requests
  case LogTraceRequest:                             return "LogTrace";
  case StatisticsRequest:                           return "Statistics";
  case LogLevelRequest:                             return "LogLevel";
  case SemStateRequest:                             return "SemState";
  case VersionRequest:                              return "Version";
  case MetricsRequest:                              return "Metrics";

  // requests enabled in DEBUG compilation
  case ExitRequest:                                 return "Exit";
  case LeakRequest:                                 return "Leak";

  case InvalidRequest:                              return "InvalidRequest";
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
  case NotifyContext:                  return "/v2/op/notify";

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

  default:
    LM_E(("Runtime Error (unclasified request %d, %s, prefix %s", rt, requestType(rt), prefix.c_str()));
    return "/unclasified";
  }
}
