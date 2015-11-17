#ifndef SRC_LIB_COMMON_STATISTICS_H_
#define SRC_LIB_COMMON_STATISTICS_H_

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
#include "common/Format.h"



/* ****************************************************************************
*
* Statistic counters for NGSI REST requests
*/
extern int noOfJsonRequests;
extern int noOfXmlRequests;
extern int noOfRegistrations;
extern int noOfRegistrationErrors;
extern int noOfRegistrationUpdates;
extern int noOfRegistrationUpdateErrors;
extern int noOfDiscoveries;
extern int noOfDiscoveryErrors;
extern int noOfAvailabilitySubscriptions;
extern int noOfAvailabilitySubscriptionErrors;
extern int noOfAvailabilityUnsubscriptions;
extern int noOfAvailabilityUnsubscriptionErrors;
extern int noOfAvailabilitySubscriptionUpdates;
extern int noOfAvailabilitySubscriptionUpdateErrors;
extern int noOfAvailabilityNotificationsReceived;
extern int noOfAvailabilityNotificationsSent;

extern int noOfQueries;
extern int noOfQueryErrors;
extern int noOfUpdates;
extern int noOfUpdateErrors;
extern int noOfSubscriptions;
extern int noOfSubscriptionErrors;
extern int noOfSubscriptionUpdates;
extern int noOfSubscriptionUpdateErrors;
extern int noOfUnsubscriptions;
extern int noOfUnsubscriptionErrors;
extern int noOfNotificationsReceived;
extern int noOfNotificationsSent;
extern int noOfQueryContextResponses;
extern int noOfUpdateContextResponses;
extern int noOfContextEntitiesByEntityId;
extern int noOfContextEntityAttributes;
extern int noOfEntityByIdAttributeByName;
extern int noOfContextEntityTypes;
extern int noOfContextEntityTypeAttributeContainer;
extern int noOfContextEntityTypeAttribute;
extern int noOfIndividualContextEntity;
extern int noOfIndividualContextEntityAttributes;
extern int noOfAttributeValueInstance;
extern int noOfIndividualContextEntityAttribute;
extern int noOfUpdateContextElement;
extern int noOfAppendContextElement;
extern int noOfUpdateContextAttribute;
extern int noOfNgsi10ContextEntityTypes;
extern int noOfNgsi10ContextEntityTypesAttributeContainer;
extern int noOfNgsi10ContextEntityTypesAttribute;
extern int noOfNgsi10SubscriptionsConvOp;
extern int noOfAllContextEntitiesRequests;
extern int noOfAllEntitiesWithTypeAndIdRequests;
extern int noOfIndividualContextEntityAttributeWithTypeAndId;
extern int noOfAttributeValueInstanceWithTypeAndId;
extern int noOfContextEntitiesByEntityIdAndType;
extern int noOfEntityByIdAttributeByNameIdAndType;

extern int noOfLogRequests;
extern int noOfVersionRequests;
extern int noOfExitRequests;
extern int noOfLeakRequests;
extern int noOfStatisticsRequests;
extern int noOfInvalidRequests;
extern int noOfRegisterResponses;

extern int noOfRtSubscribeContextAvailabilityResponse;
extern int noOfRtUpdateContextAvailabilitySubscriptionResponse;
extern int noOfRtUnsubscribeContextAvailabilityResponse;
extern int noOfRtUnsubscribeContextResponse;
extern int noOfRtSubscribeResponse;
extern int noOfRtSubscribeError;

extern int noOfSubCacheEntries;
extern int noOfSubCacheLookups;
extern int noOfSubCacheRemovals;
extern int noOfSubCacheRemovalFailures;

extern int noOfSimulatedNotifications;

/* ****************************************************************************
*
* statisticsUpdate - 
*/
extern void statisticsUpdate(RequestType request, Format inFormat);

#endif  // SRC_LIB_COMMON_STATISTICS_H_
