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
#include "common/statistics.h"
#include "common/sem.h"
#include "metricsMgr/metricsMgr.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/rest.h"
#include "serviceRoutines/statisticsTreat.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "cache/subCache.h"
#include "ngsiNotify/QueueStatistics.h"
#include "common/JsonHelper.h"




/* ****************************************************************************
*
* resetStatistics -
*/
static void resetStatistics(void)
{
  noOfJsonRequests                                = -1;
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

  noOfLogTraceRequests                            = -1;
  noOfLogLevelRequests                            = -1;
  noOfVersionRequests                             = -1;
  noOfExitRequests                                = -1;
  noOfLeakRequests                                = -1;
  noOfStatisticsRequests                          = -1;
  noOfInvalidRequests                             = -1;
  noOfRegisterResponses                           = -1;

  noOfSimulatedNotifications                      = -1;
  noOfBatchQueryRequest                           = -1;
  noOfBatchUpdateRequest                          = -1;

  QueueStatistics::reset();

  semTimeReqReset();
  semTimeTransReset();
  semTimeCacheReset();
  semTimeTimeStatReset();
  mongoPoolConnectionSemWaitingTimeReset();
  mutexTimeCCReset();

  timingStatisticsReset();
}



/* ****************************************************************************
*
* renderUsedCounter -
*/
inline void renderUsedCounter(JsonHelper& jh, const std::string& field, int counter)
{
  if (counter != -1)
  {
    jh.Int(field, counter + 1);
  }
}



/* ****************************************************************************
*
* renderCounterStats -
*/
void renderCounterStats(JsonHelper& jh)
{
  jh.StartObject();

  // FIXME: try to chose names closer to the ones used in API URLs
  renderUsedCounter(jh, "jsonRequests",                              noOfJsonRequests);
  renderUsedCounter(jh, "registrations",                             noOfRegistrations);
  renderUsedCounter(jh, "registrationUpdates",                       noOfRegistrationUpdates);
  renderUsedCounter(jh, "discoveries",                               noOfDiscoveries);
  renderUsedCounter(jh, "availabilitySubscriptions",                 noOfAvailabilitySubscriptions);
  renderUsedCounter(jh, "availabilitySubscriptionUpdates",           noOfAvailabilitySubscriptionUpdates);
  renderUsedCounter(jh, "availabilityUnsubscriptions",               noOfAvailabilityUnsubscriptions);
  renderUsedCounter(jh, "availabilityNotificationsReceived",         noOfAvailabilityNotificationsReceived);
  renderUsedCounter(jh, "availabilityNotificationsSent",             noOfAvailabilityNotificationsSent);
  renderUsedCounter(jh, "queries",                                   noOfQueries);
  renderUsedCounter(jh, "updates",                                   noOfUpdates);
  renderUsedCounter(jh, "subscriptions",                             noOfSubscriptions);
  renderUsedCounter(jh, "subscriptionUpdates",                       noOfSubscriptionUpdates);
  renderUsedCounter(jh, "unsubscriptions",                           noOfUnsubscriptions);
  renderUsedCounter(jh, "notificationsReceived",                     noOfNotificationsReceived);
  renderUsedCounter(jh, "notificationsSent",                         noOfNotificationsSent);
  renderUsedCounter(jh, "queryResponsesReceived",                    noOfQueryContextResponses);
  renderUsedCounter(jh, "updateResponsesReceived",                   noOfUpdateContextResponses);
  renderUsedCounter(jh, "contextEntitiesByEntityId",                 noOfContextEntitiesByEntityId);
  renderUsedCounter(jh, "contextEntityAttributes",                   noOfContextEntityAttributes);
  renderUsedCounter(jh, "entityByIdAttributeByName",                 noOfEntityByIdAttributeByName);
  renderUsedCounter(jh, "ctxEntityTypes",                            noOfContextEntityTypes);
  renderUsedCounter(jh, "ctxEntityTypeAttributeContainer",           noOfContextEntityTypeAttributeContainer);
  renderUsedCounter(jh, "ctxEntityTypeAttribute",                    noOfContextEntityTypeAttribute);
  renderUsedCounter(jh, "individualContextEntity",                   noOfIndividualContextEntity);
  renderUsedCounter(jh, "individualContextEntityAttributes",         noOfIndividualContextEntityAttributes);
  renderUsedCounter(jh, "individualContextEntityAttribute",          noOfIndividualContextEntityAttribute);
  renderUsedCounter(jh, "updateContextElement",                      noOfUpdateContextElement);
  renderUsedCounter(jh, "appendContextElement",                      noOfAppendContextElement);
  renderUsedCounter(jh, "updateContextAttribute",                    noOfUpdateContextAttribute);
  renderUsedCounter(jh, "ctxEntityTypesNgsi10",                      noOfNgsi10ContextEntityTypes);
  renderUsedCounter(jh, "ctxEntityTypeAttributeContainerNgsi10",     noOfNgsi10ContextEntityTypesAttributeContainer);
  renderUsedCounter(jh, "ctxEntityTypeAttributeNgsi10",              noOfNgsi10ContextEntityTypesAttribute);
  renderUsedCounter(jh, "subscriptionsNgsi10ConvOp",                 noOfNgsi10SubscriptionsConvOp);
  renderUsedCounter(jh, "allContextEntitiesRequests",                noOfAllContextEntitiesRequests);
  renderUsedCounter(jh, "allEntitiesWithTypeAndIdRequests",          noOfAllEntitiesWithTypeAndIdRequests);
  renderUsedCounter(jh, "individualCtxEntityAttributeWithTypeAndId", noOfIndividualContextEntityAttributeWithTypeAndId);
  renderUsedCounter(jh, "attributeValueInstanceWithTypeAndId",       noOfAttributeValueInstanceWithTypeAndId);
  renderUsedCounter(jh, "contextEntitiesByEntityIdAndType",          noOfContextEntitiesByEntityIdAndType);
  renderUsedCounter(jh, "entityByIdAttributeByNameIdAndType",        noOfEntityByIdAttributeByNameIdAndType);
  renderUsedCounter(jh, "batchQueryRequests",                        noOfBatchQueryRequest);
  renderUsedCounter(jh, "batchUpdateRequests",                       noOfBatchUpdateRequest);
  renderUsedCounter(jh, "logTraceRequests",                          noOfLogTraceRequests);
  renderUsedCounter(jh, "logLevelRequests",                          noOfLogLevelRequests);

  //
  // The valgrind test suite uses REST GET /version to check that the broker is alive
  // This fact makes the statistics change and some working functests fail under valgrindTestSuite
  // due to the 'extra' version-request in the statistics.
  // Instead of removing version-requests from the statistics,
  // we report the number of version-requests even if zero (-1).
  //
  jh.Int("versionRequests", noOfVersionRequests + 1);

  renderUsedCounter(jh, "exitRequests", noOfExitRequests);
  renderUsedCounter(jh, "leakRequests", noOfLeakRequests);
  renderUsedCounter(jh, "statisticsRequests", noOfStatisticsRequests);
  renderUsedCounter(jh, "invalidRequests", noOfInvalidRequests);
  renderUsedCounter(jh, "registerResponses", noOfRegisterResponses);
  renderUsedCounter(jh, "registrationErrors", noOfRegistrationErrors);
  renderUsedCounter(jh, "registrationUpdateErrors", noOfRegistrationUpdateErrors);
  renderUsedCounter(jh, "discoveryErrors", noOfDiscoveryErrors);

  jh.EndObject();
}



/* ****************************************************************************
*
* renderSemWaitStats -
*/
void renderSemWaitStats(JsonHelper& jh)
{
  jh.StartObject();
  jh.Double("request",           semTimeReqGet());
  jh.Double("dbConnectionPool",  mongoPoolConnectionSemWaitingTimeGet());
  jh.Double("transaction",       semTimeTransGet());
  jh.Double("subCache",          semTimeCacheGet());
  jh.Double("connectionContext", mutexTimeCCGet());
  jh.Double("timeStat",          semTimeTimeStatGet());
  jh.Double("metrics",           ((float) metricsMgr.semWaitTimeGet()) / 1000000);
  jh.EndObject();
}



/* ****************************************************************************
*
* renderNotifQueueStats -
*/
void renderNotifQueueStats(JsonHelper& jh)
{
  float      timeInQ = QueueStatistics::getTimeInQ();
  int        out     = QueueStatistics::getOut();

  jh.StartObject();
  jh.Int(   "in",             QueueStatistics::getIn());
  jh.Int(   "out",            out);
  jh.Int(   "reject",         QueueStatistics::getReject());
  jh.Int(   "sentOk",         QueueStatistics::getSentOK());     // FIXME P7: this needs to be generalized for all notificationModes
  jh.Int(   "sentError",      QueueStatistics::getSentError());  // FIXME P7: this needs to be generalized for all notificationModes
  jh.Double("timeInQueue",    timeInQ);
  jh.Double("avgTimeInQueue", out==0 ? 0 : (timeInQ/out));
  jh.Int(   "size",           QueueStatistics::getQSize());
  jh.EndObject();
}



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

  JsonHelper jh;

  jh.StartObject();

  if (ciP->method == "DELETE")
  {
    resetStatistics();
    jh.String("message", "All statistics counter reset");
    jh.EndObject();
    return jh.str();
  }

  /* Conditional statistics (in the same order as described in statistics.md, although
   * beautifier tools may change this order */
  if (countersStatistics)
  {
    jh.Key("counters");
    renderCounterStats(jh);
  }
  if (semWaitStatistics)
  {
    jh.Key("semWait");
    renderSemWaitStats(jh);
  }
  if (timingStatistics)
  {
    jh.Key("timing");
    renderTimingStatistics(jh);
  }
  if ((notifQueueStatistics) && (strcmp(notificationMode, "threadpool") == 0))
  {
    jh.Key("notifQueue");
    renderNotifQueueStats(jh);
  }

  // Unconditional stats
  int now = getCurrentTime();
  jh.Int("uptime_in_secs", now - startTime);
  jh.Int("measuring_interval_in_secs", now - statisticsTime);

  // Special case: simulated notifications
  int nSimNotif = __sync_fetch_and_add(&noOfSimulatedNotifications, 0);
  renderUsedCounter(jh, "simulatedNotifications", nSimNotif);

  jh.EndObject();

  ciP->httpStatusCode = SccOk;
  return jh.str();
}



/* ****************************************************************************
*
* statisticsCacheTreat -
*
*/
std::string statisticsCacheTreat
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{

  JsonHelper jh;
  jh.StartObject();

  if (ciP->method == "DELETE")
  {
    subCacheStatisticsReset("statisticsTreat::DELETE");
    jh.String("message", "All statistics counter reset");
    jh.EndObject();
    return jh.str();
  }

  //
  // mongo sub cache counters
  //
  int   mscRefreshs = 0;
  int   mscInserts  = 0;
  int   mscRemoves  = 0;
  int   mscUpdates  = 0;
  int   cacheItems  = 0;
  char  listBuffer[1024];

  cacheSemTake(__FUNCTION__, "statisticsCacheTreat");
  subCacheStatisticsGet(&mscRefreshs, &mscInserts, &mscRemoves, &mscUpdates, &cacheItems, listBuffer, sizeof(listBuffer));
  cacheSemGive(__FUNCTION__, "statisticsCacheTreat");

  jh.String("ids", listBuffer);    // FIXME P10: this seems not printing anything... is listBuffer working fine?
  jh.Int("refresh", mscRefreshs);
  jh.Int("inserts", mscInserts);
  jh.Int("removes", mscRemoves);
  jh.Int("updates", mscUpdates);
  jh.Int("items", cacheItems);

  jh.EndObject();

  ciP->httpStatusCode = SccOk;
  return jh.str();
}
