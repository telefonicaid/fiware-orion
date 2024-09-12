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
#include "rest/curlSem.h"
#include "serviceRoutines/statisticsTreat.h"
#include "mongoDriver/mongoConnectionPool.h"
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

  noOfJsonRequests             = -1;
  noOfTextRequests             = -1;
  noOfRequestsWithoutPayload   = -1;

  for (unsigned int ix = 0; ; ++ix)
  {
    noOfRequestCounters[ix].get     = -1;
    noOfRequestCounters[ix].post    = -1;
    noOfRequestCounters[ix].patch   = -1;
    noOfRequestCounters[ix].put     = -1;
    noOfRequestCounters[ix]._delete = -1;
    noOfRequestCounters[ix].options = -1;

    // We know that LeakRequest is the last request type in the array, by construction
    // FIXME P7: this is weak (but it works)
    if (noOfRequestCounters[ix].request == LeakRequest)
    {
      break;
    }
  }

  noOfInvalidRequests          = -1;
  noOfMissedVerb               = -1;
  noOfRegistrationUpdateErrors = -1;
  noOfDiscoveryErrors          = -1;
  noOfNotificationsSent        = -1;
  noOfSimulatedNotifications   = -1;

  noOfDprNgsiv1Request         = -1;
  noOfDprLegacyForwarding      = -1;
  noOfDprGeoformat             = -1;

  statisticsTime = getCurrentTime();

  QueueStatistics::reset();

  semTimeReqReset();
  semTimeTransReset();
  semTimeCacheReset();
  semTimeTimeStatReset();
  orion::mongoPoolConnectionSemWaitingTimeReset();
  mutexTimeCCReset();

  timingStatisticsReset();
}



/* ****************************************************************************
*
* renderUsedCounter -
*/
inline void renderUsedCounter(JsonObjectHelper* js, const std::string& field, int counter, bool fullCounters)
{
  if ((fullCounters) || (counter != -1))
  {
    js->addNumber(field, (long long)(counter + 1));
  }
}



/* ****************************************************************************
*
* renderCounterStats -
*/
std::string renderCounterStats(bool fullCounters)
{
  JsonObjectHelper js;

  renderUsedCounter(&js, "jsonRequests",      noOfJsonRequests, fullCounters);
  renderUsedCounter(&js, "textRequests",      noOfTextRequests, fullCounters);
  renderUsedCounter(&js, "noPayloadRequests", noOfRequestsWithoutPayload, fullCounters);

  JsonObjectHelper jsRequests;
  JsonObjectHelper jsRequestsLegacy;
  bool             touchedRequests      = false;
  bool             touchedRquestsLegacy = false;
  for (unsigned int ix = 0; ; ++ix)
  {
    JsonObjectHelper jsRequest;
    if (noOfRequestCounters[ix].getAllowed)
    {
      renderUsedCounter(&jsRequest, "GET", noOfRequestCounters[ix].get, fullCounters);
    }
    if (noOfRequestCounters[ix].postAllowed)
    {
      renderUsedCounter(&jsRequest, "POST", noOfRequestCounters[ix].post, fullCounters);
    }
    if (noOfRequestCounters[ix].patchAllowed)
    {
      renderUsedCounter(&jsRequest, "PATCH", noOfRequestCounters[ix].patch, fullCounters);
    }
    if (noOfRequestCounters[ix].putAllowed)
    {
      renderUsedCounter(&jsRequest, "PUT", noOfRequestCounters[ix].put, fullCounters);
    }
    if (noOfRequestCounters[ix].deleteAllowed)
    {
      renderUsedCounter(&jsRequest, "DELETE", noOfRequestCounters[ix]._delete, fullCounters);
    }
    if (noOfRequestCounters[ix].optionsAllowed)
    {
      renderUsedCounter(&jsRequest, "OPTIONS", noOfRequestCounters[ix].options, fullCounters);
    }

    // We add the object only in the case at least some verb has been included or if full counters has been asked
    if ((fullCounters) || (noOfRequestCounters[ix].get != -1) || (noOfRequestCounters[ix].post != -1) ||
        (noOfRequestCounters[ix].patch != -1) || (noOfRequestCounters[ix].put != -1) ||
        (noOfRequestCounters[ix]._delete != -1) || (noOfRequestCounters[ix].options != -1))
    {
      // FIXME: in 3.9.0 most of the NGSIv1/NGSI10 requests were removed. We have invented and speciall value "skip"
      // for these cases. This "skip" hack should be removed when we definitively remove all that code
      if (requestTypeForCounter(noOfRequestCounters[ix].request, std::string(noOfRequestCounters[ix].prefix)) == "skip")
      {
        continue;
      }

      // We add in the accumulator corresponing do the request kind
      if (((strncmp(noOfRequestCounters[ix].prefix, "v1", strlen("v1"))) == 0) || (strncmp(noOfRequestCounters[ix].prefix, "ngsi10", strlen("ngsi10")) == 0))
      {
        touchedRquestsLegacy = true;
        jsRequestsLegacy.addRaw(requestTypeForCounter(noOfRequestCounters[ix].request, std::string(noOfRequestCounters[ix].prefix)), jsRequest.str());
      }
      else
      {
        touchedRequests = true;
        jsRequests.addRaw(requestTypeForCounter(noOfRequestCounters[ix].request, std::string(noOfRequestCounters[ix].prefix)), jsRequest.str());
      }
    }

    // We know that LeakRequest is the last request type in the array, by construction
    // FIXME P7: this is weak (but it works)
    if (noOfRequestCounters[ix].request == LeakRequest)
    {
      break;
    }
  }
  if (touchedRequests)
  {
    js.addRaw("requests", jsRequests.str());
  }
  if (touchedRquestsLegacy)
  {
    js.addRaw("requestsLegacy", jsRequestsLegacy.str());
  }

  renderUsedCounter(&js, "missedVerb",      noOfMissedVerb, fullCounters);
  renderUsedCounter(&js, "invalidRequests", noOfInvalidRequests, fullCounters);

  renderUsedCounter(&js, "registrationUpdateErrors", noOfRegistrationUpdateErrors, fullCounters);
  renderUsedCounter(&js, "discoveryErrors", noOfDiscoveryErrors, fullCounters);
  renderUsedCounter(&js, "notificationsSent", noOfNotificationsSent, fullCounters);

  JsonObjectHelper jsDeprecated;
  renderUsedCounter(&jsDeprecated, "ngsiv1Requests", noOfDprNgsiv1Request, fullCounters);
  renderUsedCounter(&jsDeprecated, "ngsiv1Forwarding", noOfDprLegacyForwarding, fullCounters);
  renderUsedCounter(&jsDeprecated, "geoFormat", noOfDprGeoformat, fullCounters);

  std::string deprecation = jsDeprecated.str();
  if (deprecation != "{}")
  {
    js.addRaw("deprecatedFeatures", deprecation);
  }

  return js.str();
}



/* ****************************************************************************
*
* renderSemWaitStats -
*/
std::string renderSemWaitStats(void)
{
  JsonObjectHelper jh;

  jh.addNumber("request",           semTimeReqGet());
  jh.addNumber("dbConnectionPool",  orion::mongoPoolConnectionSemWaitingTimeGet());
  jh.addNumber("transaction",       semTimeTransGet());
  jh.addNumber("subCache",          semTimeCacheGet());
  jh.addNumber("connectionContext", mutexTimeCCGet());
  jh.addNumber("timeStat",          semTimeTimeStatGet());
  jh.addNumber("metrics",           ((float) metricsMgr.semWaitTimeGet()) / 1000000);

  return jh.str();
}



/* ****************************************************************************
*
* renderNotifQueueStats -
*/
std::string renderNotifQueueStats(void)
{
  JsonObjectHelper jh;
  float      timeInQ = QueueStatistics::getTimeInQ();
  int        out     = QueueStatistics::getOut();

  jh.addNumber("in",             (long long)QueueStatistics::getIn());
  jh.addNumber("out",           (long long)out);
  jh.addNumber("reject",         (long long)QueueStatistics::getReject());
  jh.addNumber("sentOk",         (long long)QueueStatistics::getSentOK());     // FIXME P7: this needs to be generalized for all notificationModes
  jh.addNumber("sentError",      (long long)QueueStatistics::getSentError());  // FIXME P7: this needs to be generalized for all notificationModes
  jh.addNumber ("timeInQueue",    timeInQ);
  jh.addNumber ("avgTimeInQueue", out==0 ? 0.0f : (timeInQ/out));
  jh.addNumber("size",           (long long)QueueStatistics::getQSize());

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
  bool fullCounters = ciP->uriParamOptions[OPT_FULL_COUNTERS];

  JsonObjectHelper js;

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
    js.addRaw("counters", renderCounterStats(fullCounters));
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
  js.addNumber("uptime_in_secs",(long long)(now - startTime));
  js.addNumber("measuring_interval_in_secs", (long long)(now - statisticsTime));

  // Special case: simulated notifications
  int nSimNotif = __sync_fetch_and_add(&noOfSimulatedNotifications, 0);
  renderUsedCounter(&js, "simulatedNotifications", nSimNotif, fullCounters);

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

  JsonObjectHelper js;

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
  js.addNumber("refresh", (long long)mscRefreshs);
  js.addNumber("inserts", (long long)mscInserts);
  js.addNumber("removes", (long long)mscRemoves);
  js.addNumber("updates", (long long)mscUpdates);
  js.addNumber("items", (long long)cacheItems);

  ciP->httpStatusCode = SccOk;
  return js.str();
}
