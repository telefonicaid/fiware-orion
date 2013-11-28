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
#include "Request.h"



/* ****************************************************************************
*
* requestType - 
*/
const char* requestType(RequestType rt)
{
  switch (rt)
  {
  case RegisterContext:                             return "RegisterContext";
  case DiscoverContextAvailability:                 return "DiscoverContextAvailability";
  case SubscribeContextAvailability:                return "SubscribeContextAvailability";
  case UpdateContextAvailabilitySubscription:       return "UpdateContextAvailabilitySubscription";
  case UnsubscribeContextAvailability:              return "UnsubscribeContextAvailability";
  case NotifyContextAvailability:                   return "NotifyContextAvailability";
  case QueryContext:                                return "QueryContext";
  case SubscribeContext:                            return "SubscribeContext";
  case UpdateContextSubscription:                   return "UpdateContextSubscription";
  case UnsubscribeContext:                          return "UnsubscribeContext";
  case NotifyContext:                               return "NotifyContext";
  case UpdateContext:                               return "UpdateContext";
  case ContextEntitiesByEntityId:                   return "ContextEntitiesByEntityId";
  case ContextEntityAttributes:                     return "ContextEntityAttributes";
  case EntityByIdAttributeByName:                   return "EntityByIdAttributeByName";
  case ContextEntityTypes:                          return "ContextEntityTypes";
  case ContextEntityTypeAttributeContainer:         return "ContextEntityTypeAttributeContainer";
  case ContextEntityTypeAttribute:                  return "ContextEntityTypeAttribute";
  case IndividualContextEntity:                     return "IndividualContextEntity";
  case IndividualContextEntityAttributes:           return "IndividualContextEntityAttributes";
  case IndividualContextEntityAttribute:            return "IndividualContextEntityAttribute";
  case UpdateContextElement:                        return "UpdateContextElement";
  case AppendContextElement:                        return "AppendContextElement";
  case UpdateContextAttribute:                      return "UpdateContextAttribute";

  case Ngsi10ContextEntityTypes:                    return "ContextEntityTypes";
  case Ngsi10ContextEntityTypesAttributeContainer:  return "ContextEntityTypesAttributeContainer";
  case Ngsi10ContextEntityTypesAttribute:           return "ContextEntityTypesAttribute";

  case LogRequest:                                  return "Log";
  case VersionRequest:                              return "Version";
  case StatisticsRequest:                           return "Statistics";
  case ExitRequest:                                 return "Exit";
  case LeakRequest:                                 return "Leak";
  case InvalidRequest:                              return "InvalidRequest";
  case RegisterResponse:                            return "RegisterResponse";
  }

  return "";
}
