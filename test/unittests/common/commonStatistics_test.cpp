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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/statistics.h"



/* ****************************************************************************
*
* statisticsUpdate - 
*
*/
TEST(commonStatistics, statisticsUpdate)
{
  noOfRegistrations                          = 0;
  noOfDiscoveries                            = 0;
  noOfAvailabilitySubscriptions              = 0;
  noOfAvailabilityUnsubscriptions            = 0;
  noOfAvailabilitySubscriptionUpdates        = 0;
  noOfAvailabilityNotificationsReceived      = 0;
  noOfQueries                                = 0;
  noOfUpdates                                = 0;
  noOfSubscriptions                          = 0;
  noOfSubscriptionUpdates                    = 0;
  noOfUnsubscriptions                        = 0;
  noOfNotificationsReceived                  = 0;
  noOfQueryContextResponses                  = 0;
  noOfUpdateContextResponses                 = 0;

  noOfContextEntitiesByEntityId              = 0;
  noOfContextEntityAttributes                = 0;
  noOfEntityByIdAttributeByName              = 0;
  noOfIndividualContextEntity                = 0;
  noOfIndividualContextEntityAttributes      = 0;
  noOfIndividualContextEntityAttribute       = 0;
  noOfUpdateContextElement                   = 0;
  noOfAppendContextElement                   = 0;
  noOfUpdateContextAttribute                 = 0;
  noOfNgsi10ContextEntityTypes               = 0;
  noOfNgsi10ContextEntityTypesAttributeContainer = 0;
  noOfNgsi10ContextEntityTypesAttribute      = 0;
  noOfNgsi10SubscriptionsConvOp              = 0;
  noOfLogRequests                            = 0;
  noOfVersionRequests                        = 0;
  noOfExitRequests                           = 0;
  noOfLeakRequests                           = 0;
  noOfStatisticsRequests                     = 0;
  noOfInvalidRequests                        = 0;
  noOfRegisterResponses                      = 0;
  noOfXmlRequests                            = 0;
  noOfJsonRequests                           = 0;

  noOfRtSubscribeContextAvailabilityResponse          = 0;
  noOfRtUpdateContextAvailabilitySubscriptionResponse = 0;
  noOfRtUnsubscribeContextAvailabilityResponse        = 0;
  noOfRtUnsubscribeContextResponse                    = 0;
  noOfRtSubscribeResponse                             = 0;
  noOfRtSubscribeError                                = 0;

  statisticsUpdate(RegisterContext, XML);
  statisticsUpdate(DiscoverContextAvailability, JSON);
  statisticsUpdate(SubscribeContextAvailability, XML);
  statisticsUpdate(UpdateContextAvailabilitySubscription, JSON);
  statisticsUpdate(UnsubscribeContextAvailability, XML);
  statisticsUpdate(NotifyContextAvailability, JSON);
  statisticsUpdate(QueryContext, XML);
  statisticsUpdate(SubscribeContext, JSON);
  statisticsUpdate(UpdateContextSubscription, XML);
  statisticsUpdate(UnsubscribeContext, JSON);
  statisticsUpdate(NotifyContext, XML);
  statisticsUpdate(UpdateContext, JSON);
  statisticsUpdate(RtQueryContextResponse, JSON);
  statisticsUpdate(RtUpdateContextResponse, XML);

  statisticsUpdate(ContextEntitiesByEntityId, XML);
  statisticsUpdate(ContextEntityAttributes, JSON);
  statisticsUpdate(EntityByIdAttributeByName, XML);
  statisticsUpdate(IndividualContextEntity, JSON);
  statisticsUpdate(IndividualContextEntityAttributes, XML);
  statisticsUpdate(AttributeValueInstance, JSON);
  statisticsUpdate(IndividualContextEntityAttribute, JSON);
  statisticsUpdate(UpdateContextElement, JSON);
  statisticsUpdate(AppendContextElement, XML);
  statisticsUpdate(UpdateContextAttribute, JSON);
  statisticsUpdate(Ngsi10ContextEntityTypes, XML);
  statisticsUpdate(Ngsi10ContextEntityTypesAttributeContainer, JSON);
  statisticsUpdate(Ngsi10ContextEntityTypesAttribute, XML);
  statisticsUpdate(Ngsi10SubscriptionsConvOp, JSON);
  statisticsUpdate(LogRequest, XML);
  statisticsUpdate(VersionRequest, JSON);
  statisticsUpdate(ExitRequest, XML);
  statisticsUpdate(LeakRequest, JSON);
  statisticsUpdate(StatisticsRequest, XML);
  statisticsUpdate(InvalidRequest, JSON);
  statisticsUpdate(RegisterResponse, XML);
  statisticsUpdate(RtSubscribeContextAvailabilityResponse, JSON);
  statisticsUpdate(RtUpdateContextAvailabilitySubscriptionResponse, XML);
  statisticsUpdate(RtUnsubscribeContextAvailabilityResponse, JSON);
  statisticsUpdate(RtUnsubscribeContextResponse, XML);
  statisticsUpdate(RtSubscribeResponse, JSON);
  statisticsUpdate(RtSubscribeError, XML);

  EXPECT_EQ(1, noOfRegistrations);
  EXPECT_EQ(1, noOfDiscoveries);
  EXPECT_EQ(1, noOfAvailabilitySubscriptions);
  EXPECT_EQ(1, noOfAvailabilityUnsubscriptions);
  EXPECT_EQ(1, noOfAvailabilitySubscriptionUpdates);
  EXPECT_EQ(1, noOfAvailabilityNotificationsReceived);
  EXPECT_EQ(1, noOfQueries);
  EXPECT_EQ(1, noOfUpdates);
  EXPECT_EQ(1, noOfSubscriptions);
  EXPECT_EQ(1, noOfSubscriptionUpdates);
  EXPECT_EQ(1, noOfUnsubscriptions);
  EXPECT_EQ(1, noOfNotificationsReceived);
  EXPECT_EQ(1, noOfQueryContextResponses);
  EXPECT_EQ(1, noOfUpdateContextResponses);
  
  EXPECT_EQ(1, noOfContextEntitiesByEntityId);
  EXPECT_EQ(1, noOfContextEntityAttributes);
  EXPECT_EQ(1, noOfEntityByIdAttributeByName);
  EXPECT_EQ(1, noOfIndividualContextEntity);
  EXPECT_EQ(1, noOfIndividualContextEntityAttributes);
  EXPECT_EQ(1, noOfIndividualContextEntityAttribute);
  EXPECT_EQ(1, noOfUpdateContextElement);
  EXPECT_EQ(1, noOfAppendContextElement);
  EXPECT_EQ(1, noOfUpdateContextAttribute);
  EXPECT_EQ(1, noOfNgsi10ContextEntityTypes);
  EXPECT_EQ(1, noOfNgsi10ContextEntityTypesAttributeContainer);
  EXPECT_EQ(1, noOfNgsi10ContextEntityTypesAttribute);
  EXPECT_EQ(1, noOfNgsi10SubscriptionsConvOp);
  EXPECT_EQ(1, noOfLogRequests);
  EXPECT_EQ(1, noOfVersionRequests);
  EXPECT_EQ(1, noOfExitRequests);
  EXPECT_EQ(1, noOfLeakRequests);
  EXPECT_EQ(1, noOfStatisticsRequests);
  EXPECT_EQ(1, noOfInvalidRequests);
  EXPECT_EQ(1, noOfRegisterResponses);
  EXPECT_EQ(1, noOfRtSubscribeContextAvailabilityResponse);
  EXPECT_EQ(1, noOfRtUpdateContextAvailabilitySubscriptionResponse);
  EXPECT_EQ(1, noOfRtUnsubscribeContextAvailabilityResponse);
  EXPECT_EQ(1, noOfRtUnsubscribeContextResponse);
  EXPECT_EQ(1, noOfRtSubscribeResponse);
  EXPECT_EQ(1, noOfRtSubscribeError);

  EXPECT_EQ(20, noOfXmlRequests);
  EXPECT_EQ(21, noOfJsonRequests);
}
