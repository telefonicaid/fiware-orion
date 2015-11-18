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
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/globals.h"
#include "common/tag.h"
#include "common/statistics.h"
#include "common/sem.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/rest.h"
#include "serviceRoutines/statisticsTreat.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoSubCache.h"

#include "ngsiNotify/QueueStatistics.h"


/* ****************************************************************************
*
* TAG_ADD - 
*/
#define TAG_ADD_COUNTER(tag, counter) valueTag(indent2, tag, counter + 1, ciP->outFormat, true)
#define TAG_ADD_STRING(tag, value)  valueTag(indent2, tag, value, ciP->outFormat, true)
#define TAG_ADD_INTEGER(tag, value, comma)  valueTag(indent2, tag, value, ciP->outFormat, comma)


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
  std::string indent2 = "  ";

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

    noOfSubCacheEntries                             = -1;
    noOfSubCacheLookups                             = -1;
    noOfSubCacheRemovals                            = -1;
    noOfSubCacheRemovalFailures                     = -1;
    noOfSimulatedNotifications                      = -1;

    semTimeReqReset();
    semTimeTransReset();
    semTimeCacheReset();
    semTimeTimeStatReset();
    mongoPoolConnectionSemWaitingTimeReset();
    mutexTimeCCReset();

    mongoSubCacheStatisticsReset("statisticsTreat::DELETE");
    timingStatisticsReset();

    out += startTag(indent, tag, "", ciP->outFormat, false, false, false);
    out += valueTag(indent2, "message", "All statistics counter reset", ciP->outFormat);
    out += endTag(indent, tag, ciP->outFormat, false, false, true, false);

    return out;
  }

  out += startTag(indent, tag, "", ciP->outFormat, false, false, false);

  if (noOfXmlRequests != -1)
  {
    out += TAG_ADD_COUNTER("xmlRequests", noOfXmlRequests);
  }

  if (noOfJsonRequests != -1)
  {
    out += TAG_ADD_COUNTER("jsonRequests", noOfJsonRequests);
  }

  if (noOfRegistrations != -1)
  {
    out += TAG_ADD_COUNTER("registrations", noOfRegistrations);
  }

  if (noOfRegistrationUpdates != -1)
  {
    out += TAG_ADD_COUNTER("registrationUpdates", noOfRegistrationUpdates);
  }

  if (noOfDiscoveries != -1)
  {
    out += TAG_ADD_COUNTER("discoveries", noOfDiscoveries);
  }

  if (noOfAvailabilitySubscriptions != -1)
  {
    out += TAG_ADD_COUNTER("availabilitySubscriptions", noOfAvailabilitySubscriptions);
  }

  if (noOfAvailabilitySubscriptionUpdates != -1)
  {
    out += TAG_ADD_COUNTER("availabilitySubscriptionUpdates", noOfAvailabilitySubscriptionUpdates);
  }

  if (noOfAvailabilityUnsubscriptions != -1)
  {
    out += TAG_ADD_COUNTER("availabilityUnsubscriptions", noOfAvailabilityUnsubscriptions);
  }

  if (noOfAvailabilityNotificationsReceived != -1)
  {
    out += TAG_ADD_COUNTER("availabilityNotificationsReceived", noOfAvailabilityNotificationsReceived);
  }

  if (noOfAvailabilityNotificationsSent != -1)
  {
    out += TAG_ADD_COUNTER("availabilityNotificationsSent", noOfAvailabilityNotificationsSent);
  }

  if (noOfQueries != -1)
  {
    out += TAG_ADD_COUNTER("queries", noOfQueries);
  }

  if (noOfUpdates != -1)
  {
    out += TAG_ADD_COUNTER("updates", noOfUpdates);
  }

  if (noOfSubscriptions != -1)
  {
    out += TAG_ADD_COUNTER("subscriptions", noOfSubscriptions);
  }

  if (noOfSubscriptionUpdates != -1)
  {
    out += TAG_ADD_COUNTER("subscriptionUpdates", noOfSubscriptionUpdates);
  }

  if (noOfUnsubscriptions != -1)
  {
    out += TAG_ADD_COUNTER("unsubscriptions", noOfUnsubscriptions);
  }

  if (noOfNotificationsReceived != -1)
  {
    out += TAG_ADD_COUNTER("notificationsReceived", noOfNotificationsReceived);
  }

  if (noOfNotificationsSent != -1)
  {
    out += TAG_ADD_COUNTER("notificationsSent", noOfNotificationsSent);
  }

  if (noOfQueryContextResponses != -1)
  {
    out += TAG_ADD_COUNTER("queryResponsesReceived", noOfQueryContextResponses);
  }

  if (noOfUpdateContextResponses != -1)
  {
    out += TAG_ADD_COUNTER("updateResponsesReceived", noOfUpdateContextResponses);
  }

  if (noOfQueryContextResponses != -1)
  {
    out += TAG_ADD_COUNTER("queryResponsesReceived", noOfQueryContextResponses);
  }

  if (noOfUpdateContextResponses != -1)
  {
    out += TAG_ADD_COUNTER("updateResponsesReceived", noOfUpdateContextResponses);
  }

  if (noOfContextEntitiesByEntityId != -1)
  {
    out += TAG_ADD_COUNTER("contextEntitiesByEntityId", noOfContextEntitiesByEntityId);
  }

  if (noOfContextEntityAttributes != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityAttributes", noOfContextEntityAttributes);
  }

  if (noOfEntityByIdAttributeByName != -1)
  {
    out += TAG_ADD_COUNTER("entityByIdAttributeByName", noOfEntityByIdAttributeByName);
  }

  if (noOfContextEntityTypes != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypes", noOfContextEntityTypes);
  }

  if (noOfContextEntityTypeAttributeContainer != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypeAttributeContainer", noOfContextEntityTypeAttributeContainer);
  }

  if (noOfContextEntityTypeAttribute != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypeAttribute", noOfContextEntityTypeAttribute);
  }


  if (noOfIndividualContextEntity != -1)
  {
    out += TAG_ADD_COUNTER("individualContextEntity", noOfIndividualContextEntity);
  }

  if (noOfIndividualContextEntityAttributes != -1)
  {
    out += TAG_ADD_COUNTER("individualContextEntityAttributes", noOfIndividualContextEntityAttributes);
  }

  if (noOfIndividualContextEntityAttribute != -1)
  {
    out += TAG_ADD_COUNTER("individualContextEntityAttribute", noOfIndividualContextEntityAttribute);
  }

  if (noOfUpdateContextElement != -1)
  {
    out += TAG_ADD_COUNTER("updateContextElement", noOfUpdateContextElement);
  }

  if (noOfAppendContextElement != -1)
  {
    out += TAG_ADD_COUNTER("appendContextElement", noOfAppendContextElement);
  }

  if (noOfUpdateContextAttribute != -1)
  {
    out += TAG_ADD_COUNTER("updateContextAttribute", noOfUpdateContextAttribute);
  }


  if (noOfNgsi10ContextEntityTypes != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypesNgsi10", noOfNgsi10ContextEntityTypes);
  }

  if (noOfNgsi10ContextEntityTypesAttributeContainer != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypeAttributeContainerNgsi10", noOfNgsi10ContextEntityTypesAttributeContainer);
  }

  if (noOfNgsi10ContextEntityTypesAttribute != -1)
  {
    out += TAG_ADD_COUNTER("contextEntityTypeAttributeNgsi10", noOfNgsi10ContextEntityTypesAttribute);
  }

  if (noOfNgsi10SubscriptionsConvOp != -1)
  {
    out += TAG_ADD_COUNTER("subscriptionsNgsi10ConvOp", noOfNgsi10SubscriptionsConvOp);
  }


  if (noOfAllContextEntitiesRequests != -1)
  {
    out += TAG_ADD_COUNTER("allContextEntitiesRequests", noOfAllContextEntitiesRequests);
  }

  if (noOfAllEntitiesWithTypeAndIdRequests != -1)
  {
    out += TAG_ADD_COUNTER("allContextEntitiesWithTypeAndIdRequests", noOfAllEntitiesWithTypeAndIdRequests);
  }

  if (noOfIndividualContextEntityAttributeWithTypeAndId != -1)
  {
    out += TAG_ADD_COUNTER("individualContextEntityAttributeWithTypeAndId", noOfIndividualContextEntityAttributeWithTypeAndId);
  }

  if (noOfAttributeValueInstanceWithTypeAndId != -1)
  {
    out += TAG_ADD_COUNTER("attributeValueInstanceWithTypeAndId", noOfAttributeValueInstanceWithTypeAndId);
  }

  if (noOfContextEntitiesByEntityIdAndType != -1)
  {
    out += TAG_ADD_COUNTER("contextEntitiesByEntityIdAndType", noOfContextEntitiesByEntityIdAndType);
  }

  if (noOfEntityByIdAttributeByNameIdAndType != -1)
  {
    out += TAG_ADD_COUNTER("entityByIdAttributeByNameIdAndType", noOfEntityByIdAttributeByNameIdAndType);
  }

  if (noOfLogRequests != -1)
  {
    out += TAG_ADD_COUNTER("logRequests", noOfLogRequests);
  }


  //
  // The valgrind test suite uses REST GET /version to check that the broker is alive
  // This fact makes the statistics change and some working functests fail under valgrindTestSuite
  // due to the 'extra' version-request in the statistics.
  // Instead of removing version-requests from the statistics,
  // we report the number of version-requests even if zero (-1).
  //
  out += TAG_ADD_COUNTER("versionRequests", noOfVersionRequests);

  if (noOfExitRequests != -1)
  {
    out += TAG_ADD_COUNTER("exitRequests", noOfExitRequests);
  }

  if (noOfLeakRequests != -1)
  {
    out += TAG_ADD_COUNTER("leakRequests", noOfLeakRequests);
  }

  if (noOfStatisticsRequests != -1)
  {
    out += TAG_ADD_COUNTER("statisticsRequests", noOfStatisticsRequests);
  }

  if (noOfInvalidRequests != -1)
  {
    out += TAG_ADD_COUNTER("invalidRequests", noOfInvalidRequests);
  }

  if (noOfRegisterResponses != -1)
  {
    out += TAG_ADD_COUNTER("registerResponses", noOfRegisterResponses);
  }


  if (noOfRegistrationErrors != -1)
  {
    out += TAG_ADD_COUNTER("registrationErrors", noOfRegistrationErrors);
  }

  if (noOfRegistrationUpdateErrors != -1)
  {
    out += TAG_ADD_COUNTER("registrationUpdateErrors", noOfRegistrationUpdateErrors);
  }

  if (noOfDiscoveryErrors != -1)
  {
    out += TAG_ADD_COUNTER("discoveryErrors", noOfDiscoveryErrors);
  }

  if (noOfSubCacheEntries != -1)
  {
    out += TAG_ADD_COUNTER("subCacheEntries", noOfSubCacheEntries);
  }

  if (noOfSubCacheLookups != -1)
  {
    out += TAG_ADD_COUNTER("subCacheLookups", noOfSubCacheLookups);
  }

  if (noOfSubCacheRemovals != -1)
  {
    out += TAG_ADD_COUNTER("subCacheRemovals", noOfSubCacheRemovals);
  }

  if (noOfSubCacheRemovalFailures != -1)
  {
    out += TAG_ADD_COUNTER("subCacheRemovalFailures", noOfSubCacheRemovalFailures);
  }


  if (semTimeStatistics)
  {
    char requestSemaphoreWaitingTime[64];
    semTimeReqGet(requestSemaphoreWaitingTime, sizeof(requestSemaphoreWaitingTime));
    out += TAG_ADD_STRING("requestSemaphoreWaitingTime", requestSemaphoreWaitingTime);

    char mongoPoolSemaphoreWaitingTime[64];
    mongoPoolConnectionSemWaitingTimeGet(mongoPoolSemaphoreWaitingTime, sizeof(mongoPoolSemaphoreWaitingTime));
    out += TAG_ADD_STRING("dbConnectionPoolWaitingTime", mongoPoolSemaphoreWaitingTime);

    char transSemaphoreWaitingTime[64];
    semTimeTransGet(transSemaphoreWaitingTime, sizeof(transSemaphoreWaitingTime));
    out += TAG_ADD_STRING("transactionSemaphoreWaitingTime", transSemaphoreWaitingTime);

    char cacheSemaphoreWaitingTime[64];
    semTimeCacheGet(cacheSemaphoreWaitingTime, sizeof(cacheSemaphoreWaitingTime));
    out += TAG_ADD_STRING("subCacheSemaphoreWaitingTime", cacheSemaphoreWaitingTime);

    char ccMutexWaitingTime[64];
    mutexTimeCCGet(ccMutexWaitingTime, sizeof(ccMutexWaitingTime));
    out += TAG_ADD_STRING("curlContextMutexWaitingTime", ccMutexWaitingTime);

    char timeStatSemaphoreWaitingTime[64];
    semTimeTimeStatGet(timeStatSemaphoreWaitingTime, sizeof(timeStatSemaphoreWaitingTime));
    out += TAG_ADD_STRING("timeStatSemaphoreWaitingTime", timeStatSemaphoreWaitingTime);
  }

  int now = getCurrentTime();
  out += valueTag(indent2, "uptime_in_secs",             now - startTime,      ciP->outFormat, true);
  out += valueTag(indent2, "measuring_interval_in_secs", now - statisticsTime, ciP->outFormat, true);


  //
  // mongo sub cache counters
  //
  int   mscRefreshs = 0;
  int   mscInserts  = 0;
  int   mscRemoves  = 0;
  int   mscUpdates  = 0;
  int   cacheItems  = 0;
  char  listBuffer[1024];
  bool  reqSemTaken;

  reqSemTake(__FUNCTION__, "mongoSubCacheStatisticsGet", SemReadOp, &reqSemTaken);
  mongoSubCacheStatisticsGet(&mscRefreshs, &mscInserts, &mscRemoves, &mscUpdates, &cacheItems, listBuffer, sizeof(listBuffer));
  reqSemGive(__FUNCTION__, "mongoSubCacheStatisticsGet", reqSemTaken);

  if (listBuffer[0] != 0)
  {
    out += TAG_ADD_STRING("subCache",          listBuffer);
  }

  std::string timingStatString      = timingStatistics(indent2, ciP->outFormat, ciP->apiVersion);
  bool        timingStat            = (timingStatString != "");
  bool        threadpool            = (strcmp(notificationMode, "threadpool") == 0);
  bool        commaAfterThreadpool  = timingStat;
  bool        commaAfterSubCache    = timingStat || threadpool;

  out += TAG_ADD_INTEGER("subCacheRefreshs", mscRefreshs, true);
  out += TAG_ADD_INTEGER("subCacheInserts",  mscInserts,  true);
  out += TAG_ADD_INTEGER("subCacheRemoves",  mscRemoves,  true);
  out += TAG_ADD_INTEGER("subCacheUpdates",  mscUpdates,  true);
  out += TAG_ADD_INTEGER("subCacheItems",    cacheItems,  commaAfterSubCache);

  if (strcmp(notificationMode, "threadpool") == 0)
  {
    out += TAG_ADD_INTEGER("noOfNotificationsQueueIn", QueueStatistics::getIn(), true);
    out += TAG_ADD_INTEGER("noOfNotificationsQueueOut", QueueStatistics::getOut(), true);
    out += TAG_ADD_INTEGER("noOfNotificationsQueueReject", QueueStatistics::getReject(), true);
    out += TAG_ADD_INTEGER("noOfNotificationsQueueSentOK", QueueStatistics::getSentOK(), true);
    out += TAG_ADD_INTEGER("noOfNotificationsQueueSentError", QueueStatistics::getSentError(), true);

    char queueTime[64];
    QueueStatistics::getTimeInQ(queueTime, sizeof(queueTime));
    out += TAG_ADD_STRING("notificationQueueTimeInQueue", queueTime);

    out += TAG_ADD_INTEGER("notificationQueueSizeSnapshot", QueueStatistics::getQSize(), commaAfterThreadpool);
  }

  if (timingStatString != "")
  {
    out += timingStatString;
  }
  {
    int nSimNotif = __sync_fetch_and_add(&noOfSimulatedNotifications, 0);
    if (nSimNotif != -1)
    {
      out += TAG_ADD_COUNTER("noOfSimulatedNotifications", nSimNotif);
    }
  }

  out += endTag(indent, tag, ciP->outFormat, false, false, true, false);

  ciP->httpStatusCode = SccOk;
  return out;
}
