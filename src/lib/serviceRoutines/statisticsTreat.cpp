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
std::string statisticsTreat(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  std::string out     = "";
  std::string tag     = "orion";
  std::string indent  = "";

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
     
     noOfContextEntitiesByEntityId            = -1;
     noOfContextEntityAttributes              = -1;
     noOfEntityByIdAttributeByName            = -1;
     noOfIndividualContextEntity              = -1;
     noOfIndividualContextEntityAttributes    = -1;
     noOfIndividualContextEntityAttribute     = -1;
     noOfUpdateContextElement                 = -1;
     noOfAppendContextElement                 = -1;
     noOfUpdateContextAttribute               = -1;
     noOfLogRequests                          = -1;
     noOfVersionRequests                      = -1;
     noOfExitRequests                         = -1;
     noOfLeakRequests                         = -1;
     noOfStatisticsRequests                   = -1;
     noOfInvalidRequests                      = -1;
     noOfRegisterResponses                    = -1;

     out += startTag(indent, tag, ciP->outFormat);
     out += valueTag(indent + "  ", "message", "All statistics counter reset", ciP->outFormat);
     out += endTag(indent, tag, ciP->outFormat);

     return out;
  }

  out += startTag(indent, tag, ciP->outFormat);

  if (noOfXmlRequests != -1)                        out += valueTag(indent + "  ", "xmlRequests",                           noOfXmlRequests + 1,                          ciP->outFormat);
  if (noOfJsonRequests != -1)                       out += valueTag(indent + "  ", "jsonRequests",                          noOfJsonRequests + 1,                         ciP->outFormat);
  if (noOfRegistrations != -1)                      out += valueTag(indent + "  ", "registrations",                         noOfRegistrations + 1,                        ciP->outFormat);
  if (noOfRegistrationUpdates != -1)                out += valueTag(indent + "  ", "registrationUpdates",                   noOfRegistrationUpdates + 1,                  ciP->outFormat);
  if (noOfDiscoveries != -1)                        out += valueTag(indent + "  ", "discoveries",                           noOfDiscoveries + 1,                          ciP->outFormat);
  if (noOfAvailabilitySubscriptions != -1)          out += valueTag(indent + "  ", "availabilitySubscriptions",             noOfAvailabilitySubscriptions + 1,            ciP->outFormat);
  if (noOfAvailabilitySubscriptionUpdates != -1)    out += valueTag(indent + "  ", "availabilitySubscriptionUpdates",       noOfAvailabilitySubscriptionUpdates + 1,      ciP->outFormat);
  if (noOfAvailabilityUnsubscriptions != -1)        out += valueTag(indent + "  ", "availabilityUnsubscriptions",           noOfAvailabilityUnsubscriptions + 1,          ciP->outFormat);
  if (noOfAvailabilityNotificationsReceived != -1)  out += valueTag(indent + "  ", "availabilityNotificationsReceived",     noOfAvailabilityNotificationsReceived + 1,    ciP->outFormat);

  if (noOfQueries != -1)                            out += valueTag(indent + "  ", "queries",                               noOfQueries + 1,                              ciP->outFormat);
  if (noOfUpdates != -1)                            out += valueTag(indent + "  ", "updates",                               noOfUpdates + 1,                              ciP->outFormat);
  if (noOfSubscriptions != -1)                      out += valueTag(indent + "  ", "subscriptions",                         noOfSubscriptions + 1,                        ciP->outFormat);
  if (noOfSubscriptionUpdates != -1)                out += valueTag(indent + "  ", "subscriptionUpdates",                   noOfSubscriptionUpdates + 1,                  ciP->outFormat);
  if (noOfUnsubscriptions != -1)                    out += valueTag(indent + "  ", "unsubscriptions",                       noOfUnsubscriptions + 1,                      ciP->outFormat);
  if (noOfNotificationsReceived != -1)              out += valueTag(indent + "  ", "notificationsReceived",                 noOfNotificationsReceived + 1,                ciP->outFormat);

  if (noOfContextEntitiesByEntityId != -1)          out += valueTag(indent + "  ", "contextEntitiesByEntityId",             noOfContextEntitiesByEntityId + 1,            ciP->outFormat);
  if (noOfContextEntityAttributes != -1)            out += valueTag(indent + "  ", "contextEntityAttributes",               noOfContextEntityAttributes + 1,              ciP->outFormat);
  if (noOfEntityByIdAttributeByName != -1)          out += valueTag(indent + "  ", "entityByIdAttributeByName",             noOfEntityByIdAttributeByName + 1,            ciP->outFormat);
  if (noOfIndividualContextEntity != -1)            out += valueTag(indent + "  ", "individualContextEntity",               noOfIndividualContextEntity + 1,              ciP->outFormat);
  if (noOfIndividualContextEntityAttributes != -1)  out += valueTag(indent + "  ", "individualContextEntityAttributes",     noOfIndividualContextEntityAttributes + 1,    ciP->outFormat);
  if (noOfIndividualContextEntityAttribute != -1)   out += valueTag(indent + "  ", "individualContextEntityAttribute",      noOfIndividualContextEntityAttribute + 1,     ciP->outFormat);
  if (noOfUpdateContextElement != -1)               out += valueTag(indent + "  ", "updateContextElement",                  noOfUpdateContextElement + 1,                 ciP->outFormat);
  if (noOfAppendContextElement != -1)               out += valueTag(indent + "  ", "appendContextElement",                  noOfAppendContextElement + 1,                 ciP->outFormat);
  if (noOfUpdateContextAttribute != -1)             out += valueTag(indent + "  ", "updateContextAttribute",                noOfUpdateContextAttribute + 1,               ciP->outFormat);
  if (noOfLogRequests != -1)                        out += valueTag(indent + "  ", "logRequests",                           noOfLogRequests + 1,                          ciP->outFormat);
  if (noOfVersionRequests != -1)                    out += valueTag(indent + "  ", "versionRequests",                       noOfVersionRequests + 1,                      ciP->outFormat);
  if (noOfExitRequests != -1)                       out += valueTag(indent + "  ", "exitRequests",                          noOfExitRequests + 1,                         ciP->outFormat);
  if (noOfLeakRequests != -1)                       out += valueTag(indent + "  ", "leakRequests",                          noOfLeakRequests + 1,                         ciP->outFormat);
  if (noOfStatisticsRequests != -1)                 out += valueTag(indent + "  ", "statisticsRequests",                    noOfStatisticsRequests + 1,                   ciP->outFormat);
  if (noOfInvalidRequests != -1)                    out += valueTag(indent + "  ", "invalidRequests",                       noOfInvalidRequests + 1,                      ciP->outFormat);
  if (noOfRegisterResponses != -1)                  out += valueTag(indent + "  ", "registerResponses",                     noOfRegisterResponses + 1,                    ciP->outFormat);

  if (noOfRegistrationErrors != -1)                 out += valueTag(indent + "  ", "registrationErrors",                    noOfRegistrationErrors + 1,                   ciP->outFormat);
  if (noOfRegistrationUpdateErrors != -1)           out += valueTag(indent + "  ", "registrationUpdateErrors",              noOfRegistrationUpdateErrors + 1,             ciP->outFormat);
  if (noOfDiscoveryErrors != -1)                    out += valueTag(indent + "  ", "discoveryErrors",                       noOfDiscoveryErrors + 1,                      ciP->outFormat);

  int now = getCurrentTime();
  out += valueTag(indent + "  ", "uptime_in_secs",             now - startTime,      ciP->outFormat);
  out += valueTag(indent + "  ", "measuring_interval_in_secs", now - statisticsTime, ciP->outFormat);

  out += endTag(indent, tag, ciP->outFormat);
  
  ciP->httpStatusCode = SccOk;
  return out;
}
