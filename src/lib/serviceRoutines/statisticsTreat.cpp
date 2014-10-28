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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/globals.h"
#include "common/tag.h"
#include "common/statistics.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/statisticsTreat.h"



/* ****************************************************************************
*
* TAG_ADD - 
*/
#define TAG_ADD(tag, counter) valueTag(indent2, tag, counter + 1, ciP->outFormat, true)



/* ****************************************************************************
*
* statisticsTreat - 
*/
std::string statisticsTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string out     = "";
  std::string tag     = "orion";
  std::string indent  = "";
  std::string indent2 = (ciP->outFormat == JSON)? indent + "    " : indent + "  ";

  if (ciP->method == "DELETE")
  {
    noOfJsonRequests                                = -1;
    noOfXmlRequests                                 = -1;
    noOfRegistrations                               = -1;
    noOfRegistrationErrors                          = -1;
    noOfRegistrationUpdates                         = -1;
    noOfRegistrationUpdateErrors                    = -1;
    noOfDiscoveries                                 = -1;
    noOfDiscoveryErrors                             = -1;
    noOfAvailabilitySubscriptions                   = -1;
    noOfAvailabilitySubscriptionErrors              = -1;
    noOfAvailabilityUnsubscriptions                 = -1;
    noOfAvailabilityUnsubscriptionErrors            = -1;
    noOfAvailabilitySubscriptionUpdates             = -1;
    noOfAvailabilitySubscriptionUpdateErrors        = -1;
    noOfAvailabilityNotificationsReceived           = -1;
    noOfAvailabilityNotificationsSent               = -1;

    noOfQueries                                     = -1;
    noOfQueryErrors                                 = -1;
    noOfUpdates                                     = -1;
    noOfUpdateErrors                                = -1;
    noOfSubscriptions                               = -1;
    noOfSubscriptionErrors                          = -1;
    noOfSubscriptionUpdates                         = -1;
    noOfSubscriptionUpdateErrors                    = -1;
    noOfUnsubscriptions                             = -1;
    noOfUnsubscriptionErrors                        = -1;
    noOfNotificationsReceived                       = -1;
    noOfNotificationsSent                           = -1;
    noOfQueryContextResponses                       = -1;
    noOfUpdateContextResponses                      = -1;

    noOfContextEntitiesByEntityId                   = -1;
    noOfContextEntityAttributes                     = -1;
    noOfEntityByIdAttributeByName                   = -1;
    noOfContextEntityTypes                          = -1;
    noOfContextEntityTypeAttributeContainer         = -1;
    noOfContextEntityTypeAttribute                  = -1;

    noOfIndividualContextEntity                     = -1;
    noOfIndividualContextEntityAttributes           = -1;
    noOfIndividualContextEntityAttribute            = -1;
    noOfUpdateContextElement                        = -1;
    noOfAppendContextElement                        = -1;
    noOfUpdateContextAttribute                      = -1;

    noOfNgsi10ContextEntityTypes                    = -1;
    noOfNgsi10ContextEntityTypesAttributeContainer  = -1;
    noOfNgsi10ContextEntityTypesAttribute           = -1;
    noOfNgsi10SubscriptionsConvOp                   = -1;

    noOfAllContextEntitiesRequests                    = -1;
    noOfAllEntitiesWithTypeAndIdRequests              = -1;
    noOfIndividualContextEntityAttributeWithTypeAndId = -1;
    noOfAttributeValueInstanceWithTypeAndId           = -1;
    noOfContextEntitiesByEntityIdAndType              = -1;
    noOfEntityByIdAttributeByNameIdAndType            = -1;

    noOfLogRequests                                 = -1;
    noOfVersionRequests                             = -1;
    noOfExitRequests                                = -1;
    noOfLeakRequests                                = -1;
    noOfStatisticsRequests                          = -1;
    noOfInvalidRequests                             = -1;
    noOfRegisterResponses                           = -1;

    out += startTag(indent, tag, ciP->outFormat, true, true);
    out += valueTag(indent2, "message", "All statistics counter reset", ciP->outFormat);
    indent2 = (ciP->outFormat == JSON)? indent + "  " : indent;
    out += endTag(indent2, tag, ciP->outFormat, false, false, true, true);
    return out;
  }

  out += startTag(indent, tag, ciP->outFormat, true, true);

  if (noOfXmlRequests != -1)
  {
    out += TAG_ADD("xmlRequests", noOfXmlRequests);
  }

  if (noOfJsonRequests != -1)
  {
    out += TAG_ADD("jsonRequests", noOfJsonRequests);
  }

  if (noOfRegistrations != -1)
  {
    out += TAG_ADD("registrations", noOfRegistrations);
  }

  if (noOfRegistrationUpdates != -1)
  {
    out += TAG_ADD("registrationUpdates", noOfRegistrationUpdates);
  }

  if (noOfDiscoveries != -1)
  {
    out += TAG_ADD("discoveries", noOfDiscoveries);
  }

  if (noOfAvailabilitySubscriptions != -1)
  {
    out += TAG_ADD("availabilitySubscriptions", noOfAvailabilitySubscriptions);
  }

  if (noOfAvailabilitySubscriptionUpdates != -1)
  {
    out += TAG_ADD("availabilitySubscriptionUpdates", noOfAvailabilitySubscriptionUpdates);
  }

  if (noOfAvailabilityUnsubscriptions != -1)
  {
    out += TAG_ADD("availabilityUnsubscriptions", noOfAvailabilityUnsubscriptions);
  }

  if (noOfAvailabilityNotificationsReceived != -1)
  {
    out += TAG_ADD("availabilityNotificationsReceived", noOfAvailabilityNotificationsReceived);
  }

  if (noOfQueries != -1)
  {
    out += TAG_ADD("queries", noOfQueries);
  }

  if (noOfUpdates != -1)
  {
    out += TAG_ADD("updates", noOfUpdates);
  }

  if (noOfSubscriptions != -1)
  {
    out += TAG_ADD("subscriptions", noOfSubscriptions);
  }

  if (noOfSubscriptionUpdates != -1)
  {
    out += TAG_ADD("subscriptionUpdates", noOfSubscriptionUpdates);
  }

  if (noOfUnsubscriptions != -1)
  {
    out += TAG_ADD("unsubscriptions", noOfUnsubscriptions);
  }

  if (noOfNotificationsReceived != -1)
  {
    out += TAG_ADD("notificationsReceived", noOfNotificationsReceived);
  }

  if (noOfQueryContextResponses != -1)
  {
    out += TAG_ADD("queryResponsesReceived", noOfQueryContextResponses);
  }

  if (noOfUpdateContextResponses != -1)
  {
    out += TAG_ADD("updateResponsesReceived", noOfUpdateContextResponses);
  }

  if (noOfQueryContextResponses != -1)
  {
    out += TAG_ADD("queryResponsesReceived", noOfQueryContextResponses);
  }

  if (noOfUpdateContextResponses != -1)
  {
    out += TAG_ADD("updateResponsesReceived", noOfUpdateContextResponses);
  }

  if (noOfContextEntitiesByEntityId != -1)
  {
    out += TAG_ADD("contextEntitiesByEntityId", noOfContextEntitiesByEntityId);
  }

  if (noOfContextEntityAttributes != -1)
  {
    out += TAG_ADD("contextEntityAttributes", noOfContextEntityAttributes);
  }

  if (noOfEntityByIdAttributeByName != -1)
  {
    out += TAG_ADD("entityByIdAttributeByName", noOfEntityByIdAttributeByName);
  }

  if (noOfContextEntityTypes != -1)
  {
    out += TAG_ADD("contextEntityTypes", noOfContextEntityTypes);
  }

  if (noOfContextEntityTypeAttributeContainer != -1)
  {
    out += TAG_ADD("contextEntityTypeAttributeContainer", noOfContextEntityTypeAttributeContainer);
  }

  if (noOfContextEntityTypeAttribute != -1)
  {
    out += TAG_ADD("contextEntityTypeAttribute", noOfContextEntityTypeAttribute);
  }


  if (noOfIndividualContextEntity != -1)
  {
    out += TAG_ADD("individualContextEntity", noOfIndividualContextEntity);
  }

  if (noOfIndividualContextEntityAttributes != -1)
  {
    out += TAG_ADD("individualContextEntityAttributes", noOfIndividualContextEntityAttributes);
  }

  if (noOfIndividualContextEntityAttribute != -1)
  {
    out += TAG_ADD("individualContextEntityAttribute", noOfIndividualContextEntityAttribute);
  }

  if (noOfUpdateContextElement != -1)
  {
    out += TAG_ADD("updateContextElement", noOfUpdateContextElement);
  }

  if (noOfAppendContextElement != -1)
  {
    out += TAG_ADD("appendContextElement", noOfAppendContextElement);
  }

  if (noOfUpdateContextAttribute != -1)
  {
    out += TAG_ADD("updateContextAttribute", noOfUpdateContextAttribute);
  }


  if (noOfNgsi10ContextEntityTypes != -1)
  {
    out += TAG_ADD("contextEntityTypesNgsi10", noOfNgsi10ContextEntityTypes);
  }

  if (noOfNgsi10ContextEntityTypesAttributeContainer != -1)
  {
    out += TAG_ADD("contextEntityTypeAttributeContainerNgsi10", noOfNgsi10ContextEntityTypesAttributeContainer);
  }

  if (noOfNgsi10ContextEntityTypesAttribute != -1)
  {
    out += TAG_ADD("contextEntityTypeAttributeNgsi10", noOfNgsi10ContextEntityTypesAttribute);
  }

  if (noOfNgsi10SubscriptionsConvOp != -1)
  {
    out += TAG_ADD("subscriptionsNgsi10ConvOp", noOfNgsi10SubscriptionsConvOp);
  }


  if (noOfAllContextEntitiesRequests != -1)
  {
    out += TAG_ADD("allContextEntitiesRequests", noOfAllContextEntitiesRequests);
  }

  if (noOfAllEntitiesWithTypeAndIdRequests != -1)
  {
    out += TAG_ADD("allContextEntitiesWithTypeAndIdRequests", noOfAllEntitiesWithTypeAndIdRequests);
  }

  if (noOfIndividualContextEntityAttributeWithTypeAndId != -1)
  {
    out += TAG_ADD("individualContextEntityAttributeWithTypeAndId", noOfIndividualContextEntityAttributeWithTypeAndId);
  }

  if (noOfAttributeValueInstanceWithTypeAndId != -1)
  {
    out += TAG_ADD("attributeValueInstanceWithTypeAndId", noOfAttributeValueInstanceWithTypeAndId);
  }

  if (noOfContextEntitiesByEntityIdAndType != -1)
  {
    out += TAG_ADD("contextEntitiesByEntityIdAndType", noOfContextEntitiesByEntityIdAndType);
  }

  if (noOfEntityByIdAttributeByNameIdAndType != -1)
  {
    out += TAG_ADD("entityByIdAttributeByNameIdAndType", noOfEntityByIdAttributeByNameIdAndType);
  }

  if (noOfLogRequests != -1)
  {
    out += TAG_ADD("logRequests", noOfLogRequests);
  }

  if (noOfVersionRequests != -1)
  {
    out += TAG_ADD("versionRequests", noOfVersionRequests);
  }

  if (noOfExitRequests != -1)
  {
    out += TAG_ADD("exitRequests", noOfExitRequests);
  }

  if (noOfLeakRequests != -1)
  {
    out += TAG_ADD("leakRequests", noOfLeakRequests);
  }

  if (noOfStatisticsRequests != -1)
  {
    out += TAG_ADD("statisticsRequests", noOfStatisticsRequests);
  }

  if (noOfInvalidRequests != -1)
  {
    out += TAG_ADD("invalidRequests", noOfInvalidRequests);
  }

  if (noOfRegisterResponses != -1)
  {
    out += TAG_ADD("registerResponses", noOfRegisterResponses);
  }


  if (noOfRegistrationErrors != -1)
  {
    out += TAG_ADD("registrationErrors", noOfRegistrationErrors);
  }

  if (noOfRegistrationUpdateErrors != -1)
  {
    out += TAG_ADD("registrationUpdateErrors", noOfRegistrationUpdateErrors);
  }

  if (noOfDiscoveryErrors != -1)
  {
    out += TAG_ADD("discoveryErrors", noOfDiscoveryErrors);
  }

  int now = getCurrentTime();
  out += valueTag(indent2, "uptime_in_secs",             now - startTime,      ciP->outFormat, true);
  out += valueTag(indent2, "measuring_interval_in_secs", now - statisticsTime, ciP->outFormat, false);

  indent2 = (ciP->outFormat == JSON)? indent + "  " : indent;
  out += endTag(indent2, tag, ciP->outFormat, false, false, true, true);

  ciP->httpStatusCode = SccOk;
  return out;
}
