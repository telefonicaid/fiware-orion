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
* statisticsTreat - 
*/
std::string statisticsTreat(ConnectionInfo* ciP, int components, std::vector<std::string>& compV, ParseData* parseDataP)
{
  std::string out     = "";
  std::string tag     = "orion";
  std::string indent  = "";
  std::string indent2 = (ciP->outFormat == JSON)? indent + "    " : indent + "  ";

  if (ciP->method == "DELETE")
  {
     noOfJsonRequests                         = -1;
     noOfXmlRequests                          = -1;
     noOfRegistrations                        = -1;
     noOfRegistrationErrors                   = -1;
     noOfRegistrationUpdates                  = -1;
     noOfRegistrationUpdateErrors             = -1;
     noOfDiscoveries                          = -1;
     noOfDiscoveryErrors                      = -1;
     noOfAvailabilitySubscriptions            = -1;
     noOfAvailabilitySubscriptionErrors       = -1;
     noOfAvailabilityUnsubscriptions          = -1;
     noOfAvailabilityUnsubscriptionErrors     = -1;
     noOfAvailabilitySubscriptionUpdates      = -1;
     noOfAvailabilitySubscriptionUpdateErrors = -1;
     noOfAvailabilityNotificationsReceived    = -1;
     noOfAvailabilityNotificationsSent        = -1;

     noOfQueries                              = -1;
     noOfQueryErrors                          = -1;
     noOfUpdates                              = -1;
     noOfUpdateErrors                         = -1;
     noOfSubscriptions                        = -1;
     noOfSubscriptionErrors                   = -1;
     noOfSubscriptionUpdates                  = -1;
     noOfSubscriptionUpdateErrors             = -1;
     noOfUnsubscriptions                      = -1;
     noOfUnsubscriptionErrors                 = -1;
     noOfNotificationsReceived                = -1;
     noOfNotificationsSent                    = -1;
     noOfQueryContextResponses                = -1;
     noOfUpdateContextResponses               = -1;

     noOfContextEntitiesByEntityId            = -1;
     noOfContextEntityAttributes              = -1;
     noOfEntityByIdAttributeByName            = -1;
     noOfContextEntityTypes                   = -1;
     noOfContextEntityTypeAttributeContainer  = -1;
     noOfContextEntityTypeAttribute           = -1;

     noOfIndividualContextEntity              = -1;
     noOfIndividualContextEntityAttributes    = -1;
     noOfIndividualContextEntityAttribute     = -1;
     noOfUpdateContextElement                 = -1;
     noOfAppendContextElement                 = -1;
     noOfUpdateContextAttribute               = -1;

     noOfNgsi10ContextEntityTypes                    = -1;
     noOfNgsi10ContextEntityTypesAttributeContainer  = -1;
     noOfNgsi10ContextEntityTypesAttribute           = -1;
     noOfNgsi10SubscriptionsConvOp                               = -1;

     noOfLogRequests                          = -1;
     noOfVersionRequests                      = -1;
     noOfExitRequests                         = -1;
     noOfLeakRequests                         = -1;
     noOfStatisticsRequests                   = -1;
     noOfInvalidRequests                      = -1;
     noOfRegisterResponses                    = -1;

     out += startTag(indent, tag, ciP->outFormat, true, true);
     out += valueTag(indent2, "message", "All statistics counter reset", ciP->outFormat);
     indent2 = (ciP->outFormat == JSON)? indent + "  " : indent;
     out += endTag(indent2, tag, ciP->outFormat, false, false, true, true);
     return out;
  }

  out += startTag(indent, tag, ciP->outFormat, true, true);

  if (noOfXmlRequests != -1)                                 out += valueTag(indent2, "xmlRequests",                                noOfXmlRequests + 1,                                 ciP->outFormat, true);
  if (noOfJsonRequests != -1)                                out += valueTag(indent2, "jsonRequests",                               noOfJsonRequests + 1,                                ciP->outFormat, true);
  if (noOfRegistrations != -1)                               out += valueTag(indent2, "registrations",                              noOfRegistrations + 1,                               ciP->outFormat, true);
  if (noOfRegistrationUpdates != -1)                         out += valueTag(indent2, "registrationUpdates",                        noOfRegistrationUpdates + 1,                         ciP->outFormat, true);
  if (noOfDiscoveries != -1)                                 out += valueTag(indent2, "discoveries",                                noOfDiscoveries + 1,                                 ciP->outFormat, true);
  if (noOfAvailabilitySubscriptions != -1)                   out += valueTag(indent2, "availabilitySubscriptions",                  noOfAvailabilitySubscriptions + 1,                   ciP->outFormat, true);
  if (noOfAvailabilitySubscriptionUpdates != -1)             out += valueTag(indent2, "availabilitySubscriptionUpdates",            noOfAvailabilitySubscriptionUpdates + 1,             ciP->outFormat, true);
  if (noOfAvailabilityUnsubscriptions != -1)                 out += valueTag(indent2, "availabilityUnsubscriptions",                noOfAvailabilityUnsubscriptions + 1,                 ciP->outFormat, true);
  if (noOfAvailabilityNotificationsReceived != -1)           out += valueTag(indent2, "availabilityNotificationsReceived",          noOfAvailabilityNotificationsReceived + 1,           ciP->outFormat, true);

  if (noOfQueries != -1)                                     out += valueTag(indent2, "queries",                                    noOfQueries + 1,                                     ciP->outFormat, true);
  if (noOfUpdates != -1)                                     out += valueTag(indent2, "updates",                                    noOfUpdates + 1,                                     ciP->outFormat, true);
  if (noOfSubscriptions != -1)                               out += valueTag(indent2, "subscriptions",                              noOfSubscriptions + 1,                               ciP->outFormat, true);
  if (noOfSubscriptionUpdates != -1)                         out += valueTag(indent2, "subscriptionUpdates",                        noOfSubscriptionUpdates + 1,                         ciP->outFormat, true);
  if (noOfUnsubscriptions != -1)                             out += valueTag(indent2, "unsubscriptions",                            noOfUnsubscriptions + 1,                             ciP->outFormat, true);
  if (noOfNotificationsReceived != -1)                       out += valueTag(indent2, "notificationsReceived",                      noOfNotificationsReceived + 1,                       ciP->outFormat, true);

  if (noOfQueryContextResponses != -1)                       out += valueTag(indent2, "queryResponsesReceived",                     noOfQueryContextResponses + 1,                       ciP->outFormat, true);
  if (noOfUpdateContextResponses != -1)                      out += valueTag(indent2, "updateResponsesReceived",                    noOfUpdateContextResponses + 1,                       ciP->outFormat, true);

  if (noOfContextEntitiesByEntityId != -1)                   out += valueTag(indent2, "contextEntitiesByEntityId",                  noOfContextEntitiesByEntityId + 1,                   ciP->outFormat, true);
  if (noOfContextEntityAttributes != -1)                     out += valueTag(indent2, "contextEntityAttributes",                    noOfContextEntityAttributes + 1,                     ciP->outFormat, true);
  if (noOfEntityByIdAttributeByName != -1)                   out += valueTag(indent2, "entityByIdAttributeByName",                  noOfEntityByIdAttributeByName + 1,                   ciP->outFormat, true);
  if (noOfContextEntityTypes != -1)                          out += valueTag(indent2, "contextEntityTypes",                         noOfContextEntityTypes + 1,                          ciP->outFormat, true);
  if (noOfContextEntityTypeAttributeContainer != -1)         out += valueTag(indent2, "contextEntityTypeAttributeContainer",        noOfContextEntityTypeAttributeContainer + 1,         ciP->outFormat, true);
  if (noOfContextEntityTypeAttribute != -1)                  out += valueTag(indent2, "contextEntityTypeAttribute",                 noOfContextEntityTypeAttribute + 1,                  ciP->outFormat, true);

  if (noOfIndividualContextEntity != -1)                     out += valueTag(indent2, "individualContextEntity",                    noOfIndividualContextEntity + 1,                     ciP->outFormat, true);
  if (noOfIndividualContextEntityAttributes != -1)           out += valueTag(indent2, "individualContextEntityAttributes",          noOfIndividualContextEntityAttributes + 1,           ciP->outFormat, true);
  if (noOfIndividualContextEntityAttribute != -1)            out += valueTag(indent2, "individualContextEntityAttribute",           noOfIndividualContextEntityAttribute + 1,            ciP->outFormat, true);
  if (noOfUpdateContextElement != -1)                        out += valueTag(indent2, "updateContextElement",                       noOfUpdateContextElement + 1,                        ciP->outFormat, true);
  if (noOfAppendContextElement != -1)                        out += valueTag(indent2, "appendContextElement",                       noOfAppendContextElement + 1,                        ciP->outFormat, true);
  if (noOfUpdateContextAttribute != -1)                      out += valueTag(indent2, "updateContextAttribute",                     noOfUpdateContextAttribute + 1,                      ciP->outFormat, true);

  if (noOfNgsi10ContextEntityTypes != -1)                    out += valueTag(indent2, "contextEntityTypesNgsi10",                   noOfNgsi10ContextEntityTypes + 1,                    ciP->outFormat, true);
  if (noOfNgsi10ContextEntityTypesAttributeContainer != -1)  out += valueTag(indent2, "contextEntityTypeAttributeContainerNgsi10",  noOfNgsi10ContextEntityTypesAttributeContainer + 1,  ciP->outFormat, true);
  if (noOfNgsi10ContextEntityTypesAttribute != -1)           out += valueTag(indent2, "contextEntityTypeAttributeNgsi10",           noOfNgsi10ContextEntityTypesAttribute + 1,           ciP->outFormat, true);
  if (noOfNgsi10SubscriptionsConvOp != -1)                   out += valueTag(indent2, "subscriptionsNgsi10ConvOp",                  noOfNgsi10SubscriptionsConvOp + 1,                   ciP->outFormat, true);

  if (noOfLogRequests != -1)                                 out += valueTag(indent2, "logRequests",                                noOfLogRequests + 1,                                 ciP->outFormat, true);
  if (noOfVersionRequests != -1)                             out += valueTag(indent2, "versionRequests",                            noOfVersionRequests + 1,                             ciP->outFormat, true);
  if (noOfExitRequests != -1)                                out += valueTag(indent2, "exitRequests",                               noOfExitRequests + 1,                                ciP->outFormat, true);
  if (noOfLeakRequests != -1)                                out += valueTag(indent2, "leakRequests",                               noOfLeakRequests + 1,                                ciP->outFormat, true);
  if (noOfStatisticsRequests != -1)                          out += valueTag(indent2, "statisticsRequests",                         noOfStatisticsRequests + 1,                          ciP->outFormat, true);
  if (noOfInvalidRequests != -1)                             out += valueTag(indent2, "invalidRequests",                            noOfInvalidRequests + 1,                             ciP->outFormat, true);
  if (noOfRegisterResponses != -1)                           out += valueTag(indent2, "registerResponses",                          noOfRegisterResponses + 1,                           ciP->outFormat, true);

  if (noOfRegistrationErrors != -1)                          out += valueTag(indent2, "registrationErrors",                         noOfRegistrationErrors + 1,                          ciP->outFormat, true);
  if (noOfRegistrationUpdateErrors != -1)                    out += valueTag(indent2, "registrationUpdateErrors",                   noOfRegistrationUpdateErrors + 1,                    ciP->outFormat, true);
  if (noOfDiscoveryErrors != -1)                             out += valueTag(indent2, "discoveryErrors",                            noOfDiscoveryErrors + 1,                             ciP->outFormat, true);

  int now = getCurrentTime();
  out += valueTag(indent2, "uptime_in_secs",             now - startTime,      ciP->outFormat, true);
  out += valueTag(indent2, "measuring_interval_in_secs", now - statisticsTime, ciP->outFormat, false);

  indent2 = (ciP->outFormat == JSON)? indent + "  " : indent;
  out += endTag(indent2, tag, ciP->outFormat, false, false, true, true);
  
  ciP->httpStatusCode = SccOk;
  return out;
}
