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
* TAG_ADD -
*/
#define TAG_ADD_COUNTER(tag, counter) valueTag(indent2, tag, counter + 1, ciP->outtrue)
#define TAG_ADD_STRING(tag, value)  valueTag(indent2, tag, value, ciP->outtrue)
#define TAG_ADD_INTEGER(tag, value, comma)  valueTag(indent2, tag, value, ciP->outMimeType, comma)



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
  noOfRegistrationRequest                         = -1;
  noOfRegistrationsRequest                        = -1;

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
inline void renderUsedCounter(JsonHelper* js, const std::string& field, int counter)
{
  if (counter != -1)
  {
    js->addNumber(field, counter + 1);
  }
}



/* ****************************************************************************
*
* renderCounterStats -
*/
std::string renderCounterStats(void)
{
  JsonHelper js;

  // FIXME: try to chose names closer to the ones used in API URLs
  renderUsedCounter(&js, "jsonRequests",                              noOfJsonRequests);
  renderUsedCounter(&js, "registrations",                             noOfRegistrations);
  renderUsedCounter(&js, "registrationUpdates",                       noOfRegistrationUpdates);
  renderUsedCounter(&js, "discoveries",                               noOfDiscoveries);
  renderUsedCounter(&js, "availabilitySubscriptions",                 noOfAvailabilitySubscriptions);
  renderUsedCounter(&js, "availabilitySubscriptionUpdates",           noOfAvailabilitySubscriptionUpdates);
  renderUsedCounter(&js, "availabilityUnsubscriptions",               noOfAvailabilityUnsubscriptions);
  renderUsedCounter(&js, "availabilityNotificationsReceived",         noOfAvailabilityNotificationsReceived);
  renderUsedCounter(&js, "availabilityNotificationsSent",             noOfAvailabilityNotificationsSent);
  renderUsedCounter(&js, "queries",                                   noOfQueries);
  renderUsedCounter(&js, "updates",                                   noOfUpdates);
  renderUsedCounter(&js, "subscriptions",                             noOfSubscriptions);
  renderUsedCounter(&js, "subscriptionUpdates",                       noOfSubscriptionUpdates);
  renderUsedCounter(&js, "unsubscriptions",                           noOfUnsubscriptions);
  renderUsedCounter(&js, "notificationsReceived",                     noOfNotificationsReceived);
  renderUsedCounter(&js, "notificationsSent",                         noOfNotificationsSent);
  renderUsedCounter(&js, "queryResponsesReceived",                    noOfQueryContextResponses);
  renderUsedCounter(&js, "updateResponsesReceived",                   noOfUpdateContextResponses);
  renderUsedCounter(&js, "contextEntitiesByEntityId",                 noOfContextEntitiesByEntityId);
  renderUsedCounter(&js, "contextEntityAttributes",                   noOfContextEntityAttributes);
  renderUsedCounter(&js, "entityByIdAttributeByName",                 noOfEntityByIdAttributeByName);
  renderUsedCounter(&js, "ctxEntityTypes",                            noOfContextEntityTypes);
  renderUsedCounter(&js, "ctxEntityTypeAttributeContainer",           noOfContextEntityTypeAttributeContainer);
  renderUsedCounter(&js, "ctxEntityTypeAttribute",                    noOfContextEntityTypeAttribute);
  renderUsedCounter(&js, "individualContextEntity",                   noOfIndividualContextEntity);
  renderUsedCounter(&js, "individualContextEntityAttributes",         noOfIndividualContextEntityAttributes);
  renderUsedCounter(&js, "individualContextEntityAttribute",          noOfIndividualContextEntityAttribute);
  renderUsedCounter(&js, "updateContextElement",                      noOfUpdateContextElement);
  renderUsedCounter(&js, "appendContextElement",                      noOfAppendContextElement);
  renderUsedCounter(&js, "updateContextAttribute",                    noOfUpdateContextAttribute);
  renderUsedCounter(&js, "ctxEntityTypesNgsi10",                      noOfNgsi10ContextEntityTypes);
  renderUsedCounter(&js, "ctxEntityTypeAttributeContainerNgsi10",     noOfNgsi10ContextEntityTypesAttributeContainer);
  renderUsedCounter(&js, "ctxEntityTypeAttributeNgsi10",              noOfNgsi10ContextEntityTypesAttribute);
  renderUsedCounter(&js, "subscriptionsNgsi10ConvOp",                 noOfNgsi10SubscriptionsConvOp);
  renderUsedCounter(&js, "allContextEntitiesRequests",                noOfAllContextEntitiesRequests);
  renderUsedCounter(&js, "allEntitiesWithTypeAndIdRequests",          noOfAllEntitiesWithTypeAndIdRequests);
  renderUsedCounter(&js, "individualCtxEntityAttributeWithTypeAndId", noOfIndividualContextEntityAttributeWithTypeAndId);
  renderUsedCounter(&js, "attributeValueInstanceWithTypeAndId",       noOfAttributeValueInstanceWithTypeAndId);
  renderUsedCounter(&js, "contextEntitiesByEntityIdAndType",          noOfContextEntitiesByEntityIdAndType);
  renderUsedCounter(&js, "entityByIdAttributeByNameIdAndType",        noOfEntityByIdAttributeByNameIdAndType);
  renderUsedCounter(&js, "batchQueryRequests",                        noOfBatchQueryRequest);
  renderUsedCounter(&js, "batchUpdateRequests",                       noOfBatchUpdateRequest);
  renderUsedCounter(&js, "registrationRequest",                       noOfRegistrationRequest);
  renderUsedCounter(&js, "registrationsRequest",                      noOfRegistrationsRequest);
  renderUsedCounter(&js, "logTraceRequests",                          noOfLogTraceRequests);
  renderUsedCounter(&js, "logLevelRequests",                          noOfLogLevelRequests);

  //
  // The valgrind test suite uses REST GET /version to check that the broker is alive
  // This fact makes the statistics change and some working functests fail under valgrindTestSuite
  // due to the 'extra' version-request in the statistics.
  // Instead of removing version-requests from the statistics,
  // we report the number of version-requests even if zero (-1).
  //
  js.addNumber("versionRequests", noOfVersionRequests + 1);

  renderUsedCounter(&js, "exitRequests", noOfExitRequests);
  renderUsedCounter(&js, "leakRequests", noOfLeakRequests);
  renderUsedCounter(&js, "statisticsRequests", noOfStatisticsRequests);
  renderUsedCounter(&js, "invalidRequests", noOfInvalidRequests);
  renderUsedCounter(&js, "registerResponses", noOfRegisterResponses);
  renderUsedCounter(&js, "registrationErrors", noOfRegistrationErrors);
  renderUsedCounter(&js, "registrationUpdateErrors", noOfRegistrationUpdateErrors);
  renderUsedCounter(&js, "discoveryErrors", noOfDiscoveryErrors);

  return js.str();
}



/* ****************************************************************************
*
* renderSemWaitStats -
*/
std::string renderSemWaitStats(void)
{
  JsonHelper jh;

  jh.addFloat("request",           semTimeReqGet());
  jh.addFloat("dbConnectionPool",  mongoPoolConnectionSemWaitingTimeGet());
  jh.addFloat("transaction",       semTimeTransGet());
  jh.addFloat("subCache",          semTimeCacheGet());
  jh.addFloat("connectionContext", mutexTimeCCGet());
  jh.addFloat("timeStat",          semTimeTimeStatGet());
  jh.addFloat("metrics",           ((float) metricsMgr.semWaitTimeGet()) / 1000000);

  return jh.str();
}



/* ****************************************************************************
*
* renderNotifQueueStats -
*/
std::string renderNotifQueueStats(void)
{
  JsonHelper jh;
  float      timeInQ = QueueStatistics::getTimeInQ();
  int        out     = QueueStatistics::getOut();

  jh.addNumber("in",             QueueStatistics::getIn());
  jh.addNumber("out",            out);
  jh.addNumber("reject",         QueueStatistics::getReject());
  jh.addNumber("sentOk",         QueueStatistics::getSentOK());     // FIXME P7: this needs to be generalized for all notificationModes
  jh.addNumber("sentError",      QueueStatistics::getSentError());  // FIXME P7: this needs to be generalized for all notificationModes
  jh.addFloat ("timeInQueue",    timeInQ);
  jh.addFloat ("avgTimeInQueue", out==0 ? 0 : (timeInQ/out));
  jh.addNumber("size",           QueueStatistics::getQSize());

  return jh.str();
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

  JsonHelper js;

  if (ciP->method == "DELETE")
  {
    resetStatistics();
    js.addString("message", "All statistics counter reset");
    return js.str();
  }

  /* Conditional statistics (in the same order as described in statistics.md, although
   * beautifier tools may change this order */
  if (countersStatistics)
  {
    js.addRaw("counters", renderCounterStats());
  }
  if (semWaitStatistics)
  {
    js.addRaw("semWait", renderSemWaitStats());
  }
  if (timingStatistics)
  {
    js.addRaw("timing", renderTimingStatistics());
  }
  if ((notifQueueStatistics) && (strcmp(notificationMode, "threadpool") == 0))
  {
    js.addRaw("notifQueue", renderNotifQueueStats());
  }

  // Unconditional stats
  int now = getCurrentTime();
  js.addNumber("uptime_in_secs", now - startTime);
  js.addNumber("measuring_interval_in_secs", now - statisticsTime);

  // Special case: simulated notifications
  int nSimNotif = __sync_fetch_and_add(&noOfSimulatedNotifications, 0);
  renderUsedCounter(&js, "simulatedNotifications", nSimNotif);

  ciP->httpStatusCode = SccOk;
  return js.str();
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

  JsonHelper js;

  if (ciP->method == "DELETE")
  {
    subCacheStatisticsReset("statisticsTreat::DELETE");
    js.addString("message", "All statistics counter reset");
    return js.str();
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

  js.addString("ids", listBuffer);    // FIXME P10: this seems not printing anything... is listBuffer working fine?
  js.addNumber("refresh", mscRefreshs);
  js.addNumber("inserts", mscInserts);
  js.addNumber("removes", mscRemoves);
  js.addNumber("updates", mscUpdates);
  js.addNumber("items", cacheItems);

  ciP->httpStatusCode = SccOk;
  return js.str();
}
