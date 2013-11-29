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
#include "common/statistics.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* Statistic counters for NGSI REST requests
*/
int noOfJsonRequests                         = -1;
int noOfXmlRequests                          = -1;
int noOfRegistrations                        = -1;
int noOfRegistrationErrors                   = -1;
int noOfRegistrationUpdates                  = -1;
int noOfRegistrationUpdateErrors             = -1;
int noOfDiscoveries                          = -1;
int noOfDiscoveryErrors                      = -1;
int noOfAvailabilitySubscriptions            = -1;
int noOfAvailabilitySubscriptionErrors       = -1;
int noOfAvailabilityUnsubscriptions          = -1;
int noOfAvailabilityUnsubscriptionErrors     = -1;
int noOfAvailabilitySubscriptionUpdates      = -1;
int noOfAvailabilitySubscriptionUpdateErrors = -1;
int noOfAvailabilityNotificationsReceived    = -1;
int noOfAvailabilityNotificationsSent        = -1;

int noOfQueries                              = -1;
int noOfQueryErrors                          = -1;
int noOfUpdates                              = -1;
int noOfUpdateErrors                         = -1;
int noOfSubscriptions                        = -1;
int noOfSubscriptionErrors                   = -1;
int noOfSubscriptionUpdates                  = -1;
int noOfSubscriptionUpdateErrors             = -1;
int noOfUnsubscriptions                      = -1;
int noOfUnsubscriptionErrors                 = -1;
int noOfNotificationsReceived                = -1;
int noOfNotificationsSent                    = -1;

int noOfContextEntitiesByEntityId            = -1;
int noOfContextEntityAttributes              = -1;
int noOfEntityByIdAttributeByName            = -1;
int noOfContextEntityTypes                   = -1;
int noOfContextEntityTypeAttributeContainer  = -1;
int noOfContextEntityTypeAttribute           = -1;

int noOfIndividualContextEntity                     = -1;
int noOfIndividualContextEntityAttributes           = -1;
int noOfIndividualContextEntityAttribute            = -1;
int noOfNgsi10ContextEntityTypes                    = -1;
int noOfNgsi10ContextEntityTypesAttributeContainer  = -1;
int noOfNgsi10ContextEntityTypesAttribute           = -1;

int noOfUpdateContextElement                 = -1;
int noOfAppendContextElement                 = -1;
int noOfUpdateContextAttribute               = -1;

int noOfLogRequests                          = -1;
int noOfVersionRequests                      = -1;
int noOfExitRequests                         = -1;
int noOfLeakRequests                         = -1;
int noOfStatisticsRequests                   = -1;
int noOfInvalidRequests                      = -1;
int noOfRegisterResponses                    = -1;



/* ****************************************************************************
*
* statisticsUpdate - 
*/
void statisticsUpdate(RequestType request, Format inFormat)
{
   if (inFormat == XML)     ++noOfXmlRequests;
   if (inFormat == JSON)    ++noOfJsonRequests;

   switch (request)
   {
   case RegisterContext:                        ++noOfRegistrations; break;
   case DiscoverContextAvailability:            ++noOfDiscoveries; break;
   case SubscribeContextAvailability:           ++noOfAvailabilitySubscriptions; break;
   case UpdateContextAvailabilitySubscription:  ++noOfAvailabilitySubscriptionUpdates; break;
   case UnsubscribeContextAvailability:         ++noOfAvailabilityUnsubscriptions; break;
   case NotifyContextAvailability:              ++noOfAvailabilityNotificationsReceived; break;

   case QueryContext:                           ++noOfQueries; break;
   case SubscribeContext:                       ++noOfSubscriptions; break;
   case UpdateContextSubscription:              ++noOfSubscriptionUpdates; break;
   case UnsubscribeContext:                     ++noOfUnsubscriptions; break;
   case NotifyContext:                          ++noOfNotificationsReceived; break;
   case UpdateContext:                          ++noOfUpdates; break;

   case ContextEntitiesByEntityId:              ++noOfContextEntitiesByEntityId; break;
   case ContextEntityAttributes:                ++noOfContextEntityAttributes; break;
   case EntityByIdAttributeByName:              ++noOfEntityByIdAttributeByName; break;
   case ContextEntityTypes:                     ++noOfContextEntityTypes; break;
   case ContextEntityTypeAttributeContainer:    ++noOfContextEntityTypeAttributeContainer; break;
   case ContextEntityTypeAttribute:             ++noOfContextEntityTypeAttribute; break;
   case IndividualContextEntity:                ++noOfIndividualContextEntity; break;
   case IndividualContextEntityAttributes:      ++noOfIndividualContextEntityAttributes; break;
   case IndividualContextEntityAttribute:       ++noOfIndividualContextEntityAttribute; break;

   case UpdateContextElement:                       ++noOfUpdateContextElement; break;
   case AppendContextElement:                       ++noOfAppendContextElement; break;
   case UpdateContextAttribute:                     ++noOfUpdateContextAttribute; break;
   case Ngsi10ContextEntityTypes:                   ++noOfNgsi10ContextEntityTypes; break;
   case Ngsi10ContextEntityTypesAttributeContainer: ++noOfNgsi10ContextEntityTypesAttributeContainer; break;
   case Ngsi10ContextEntityTypesAttribute:          ++noOfNgsi10ContextEntityTypesAttribute; break;

   case LogRequest:                             ++noOfLogRequests; break;
   case VersionRequest:                         ++noOfVersionRequests; break;
   case ExitRequest:                            ++noOfExitRequests; break;
   case LeakRequest:                            ++noOfLeakRequests; break;
   case StatisticsRequest:                      ++noOfStatisticsRequests; break;

   case InvalidRequest:                         ++noOfInvalidRequests; break;
   case RegisterResponse:                       ++noOfRegisterResponses; break;
   }
}
