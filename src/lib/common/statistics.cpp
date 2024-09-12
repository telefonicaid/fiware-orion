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
#include "common/statistics.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "logMsg/logMsg.h"
#include "common/JsonHelper.h"



/* ****************************************************************************
*
* Statistic time counters -
*/
TimeStat           accTimeStat;
TimeStat           lastTimeStat;
__thread TimeStat  threadLastTimeStat;



/* ****************************************************************************
*
* Statistic counters for NGSI REST requests
*/
// By content
int noOfJsonRequests           = -1;
int noOfTextRequests           = -1;
int noOfRequestsWithoutPayload = -1;

// By url
// FIXME P3: EntityRequest is used both tor /v2/entities/{id} and /v2/entities/{id}/attrs although
// not the same verbs are allowed in both. Thus, counters are not perfect in that case but I think
// we can live with it...
UrlCounter noOfRequestCounters[] =
{
  // v2                                                         GET    POST   PATCH  PUT    DELET  OPT
  {EntryPointsRequest,            "v2", -1, -1, -1, -1, -1, -1, true,  false, false, false ,false, true},
  {EntitiesRequest,               "v2", -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {EntityRequest,                 "v2", -1, -1, -1, -1, -1, -1, true,  true,  true,  true,  true,  true},
  {EntityAttributeRequest,        "v2", -1, -1, -1, -1, -1, -1, true,  false, false, true,  true,  true},
  {EntityAttributeValueRequest,   "v2", -1, -1, -1, -1, -1, -1, true,  false, false, true,  false, true},
  {EntityAllTypesRequest,         "v2", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, true},
  {EntityTypeRequest,             "v2", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, true},
  {SubscriptionsRequest,          "v2", -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {SubscriptionRequest,           "v2", -1, -1, -1, -1, -1, -1, true,  false, true,  false, true,  true},
  {RegistrationsRequest,          "v2", -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {RegistrationRequest,           "v2", -1, -1, -1, -1, -1, -1, true,  false, true,  false, true,  true},
  {BatchQueryRequest,             "v2", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, true},
  {BatchUpdateRequest,            "v2", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, true},
  {NotifyContext,                 "v2",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},

  {LogTraceRequest,               "log", -1, -1, -1, -1, -1, -1, true,  false, false, true,  true,  false},
  {StatisticsRequest,             "statistics", -1, -1, -1, -1, -1, -1, true,  false, false, false, true,  false},
  {StatisticsRequest,             "cache", -1, -1, -1, -1, -1, -1, true,  false, false, false, true,  false},
  {LogLevelRequest,               "admin", -1, -1, -1, -1, -1, -1, true,  false, false, true,  false, false},
  {SemStateRequest,               "admin", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  {VersionRequest,                "version", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  {MetricsRequest,                "admin", -1, -1, -1, -1, -1, -1, true,  false, false, false, true,  false},

  // FIXME: disable unused NGSv1 API routes in Orion 3.9.0, to be definetively removed at some point of the future
  // v1 and ngsi10 legacy                                                           GET    POST   PATCH  PUT    DELET  OPT
  //{ContextEntitiesByEntityId,                     "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{ContextEntityAttributes,                       "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{EntityByIdAttributeByName,                     "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{ContextEntityTypes,                            "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{ContextEntityTypeAttributeContainer,           "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{ContextEntityTypeAttribute,                    "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},

  //{IndividualContextEntity,                       "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  //{IndividualContextEntity,                       "ngsi10", -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  {IndividualContextEntity,                       "v1",     -1, -1, -1, -1, -1, -1, false,  false,  false, true,  true,  false},

  //{IndividualContextEntityAttributes,             "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  //{IndividualContextEntityAttributes,             "ngsi10", -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},ç

  //{IndividualContextEntityAttribute,              "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  //{IndividualContextEntityAttribute,              "ngsi10", -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  {IndividualContextEntityAttribute,              "v1",     -1, -1, -1, -1, -1, -1, true,  false,  false, false,  false,  false},

  //{Ngsi10ContextEntityTypes,                      "v1",     -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  //{Ngsi10ContextEntityTypes,                      "ngsi10", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},

  //{Ngsi10ContextEntityTypesAttributeContainer,    "v1",     -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  //{Ngsi10ContextEntityTypesAttributeContainer,    "ngsi10", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},

  //{Ngsi10ContextEntityTypesAttribute,             "v1",     -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  //{Ngsi10ContextEntityTypesAttribute,             "ngsi10", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},

  //{Ngsi10SubscriptionsConvOp,                     "v1",     -1, -1, -1, -1, -1, -1, false,  false, false, true, true,  false},
  //{Ngsi10SubscriptionsConvOp,                     "ngsi10", -1, -1, -1, -1, -1, -1, false,  false, false, true, true,  false},

  //{EntityTypes,                                   "v1",     -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  //{AttributesForEntityType,                       "v1",     -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  //{AllContextEntities,                            "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{AllEntitiesWithTypeAndId,                      "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  //{IndividualContextEntityAttributeWithTypeAndId, "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, true,  true,  false},
  //{ContextEntitiesByEntityIdAndType,              "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},
  //{EntityByIdAttributeByNameIdAndType,            "v1",     -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, false},

  //{RegisterContext,                               "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  //{DiscoverContextAvailability,                   "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},

  {UpdateContext,                                 "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  {UpdateContext,                                 "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  {QueryContext,                                  "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  {QueryContext,                                  "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  //{SubscribeContext,                              "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},  // two URLs: subscribeContext and contextSubscriptions
  //{SubscribeContext,                              "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},  // two URLs: subscribeContext and contextSubscriptions
  //{UpdateContextSubscription,                     "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  //{UpdateContextSubscription,                     "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  //{UnsubscribeContext,                            "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},
  //{UnsubscribeContext,                            "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},

  //{NotifyContext,                                 "v1",     -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},  // also in v2
  //{NotifyContext,                                 "ngsi10", -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},  // also in v2

  // Special ones (LeakRequest MUST be always the last one in the array. See statisticsUpdate() and resetStatistics() comments
  {ExitRequest,                   "exit", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  {LeakRequest,                   "leak", -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false}
};

// Special
int noOfInvalidRequests          = -1;
int noOfMissedVerb               = -1;
int noOfRegistrationUpdateErrors = -1;
int noOfDiscoveryErrors          = -1;
int noOfNotificationsSent        = -1;
int noOfSimulatedNotifications   = -1;

// Deprecated features
int noOfDprNgsiv1Request         = -1;
int noOfDprLegacyForwarding      = -1;
int noOfDprGeoformat             = -1;


/* ****************************************************************************
*
* timeSpecToFloat -
*
*/
inline float timeSpecToFloat(const struct timespec& t)
{
  return t.tv_sec + ((float) t.tv_nsec) / 1E9;
}



/* ****************************************************************************
*
* renderTimingStatistics -
*
* xxxReqTime           - the total time that the LAST request took.
*                        Measuring from the first MHD callback to 'connectionTreat',
*                        until the MHD callback to 'requestCompleted'.
* xxxJsonV1ParseTime   - the time that the JSON parse+treat of the LAST request took.
* xxxJsonV2ParseTime   - the time that the JSON parse+treat of the LAST request took.
* xxxMongoBackendTime  - the time that the mongoBackend took to treat the last request
* xxxReadWaitTime      - 
* xxxWriteWaitTime     - 
* xxxCommandWaitTime   - 
* xxxRenderTime        - the time that the last render took to render the response
*
*/
std::string renderTimingStatistics(void)
{

  timeStatSemTake(__FUNCTION__, "putting stats together");

  bool accJsonV1ParseTime      = (accTimeStat.jsonV1ParseTime.tv_sec != 0)        || (accTimeStat.jsonV1ParseTime.tv_nsec != 0);
  bool accJsonV2ParseTime      = (accTimeStat.jsonV2ParseTime.tv_sec != 0)        || (accTimeStat.jsonV2ParseTime.tv_nsec != 0);
  bool accMongoBackendTime     = (accTimeStat.mongoBackendTime.tv_sec != 0)       || (accTimeStat.mongoBackendTime.tv_nsec != 0);
  bool accMongoReadWaitTime    = (accTimeStat.mongoReadWaitTime.tv_sec != 0)      || (accTimeStat.mongoReadWaitTime.tv_nsec != 0);
  bool accMongoWriteWaitTime   = (accTimeStat.mongoWriteWaitTime.tv_sec != 0)     || (accTimeStat.mongoWriteWaitTime.tv_nsec != 0);
  bool accMongoCommandWaitTime = (accTimeStat.mongoCommandWaitTime.tv_sec != 0)   || (accTimeStat.mongoCommandWaitTime.tv_nsec != 0);
  bool accExprBasicCtxBldTime  = (accTimeStat.exprBasicCtxBldTime.tv_sec != 0)    || (accTimeStat.exprBasicCtxBldTime.tv_nsec != 0);
  bool accExprBasicEvalTime    = (accTimeStat.exprBasicEvalTime.tv_sec != 0)      || (accTimeStat.exprBasicEvalTime.tv_nsec != 0);
  bool accExprJexlCtxBldTime   = (accTimeStat.exprJexlCtxBldTime.tv_sec != 0)     || (accTimeStat.exprJexlCtxBldTime.tv_nsec != 0);
  bool accExprJexlEvalTime     = (accTimeStat.exprJexlEvalTime.tv_sec != 0)       || (accTimeStat.exprJexlEvalTime.tv_nsec != 0);
  bool accRenderTime           = (accTimeStat.renderTime.tv_sec != 0)             || (accTimeStat.renderTime.tv_nsec != 0);
  bool accReqTime              = (accTimeStat.reqTime.tv_sec != 0)                || (accTimeStat.reqTime.tv_nsec != 0);

  bool lastJsonV1ParseTime      = (lastTimeStat.jsonV1ParseTime.tv_sec != 0)      || (lastTimeStat.jsonV1ParseTime.tv_nsec != 0);
  bool lastJsonV2ParseTime      = (lastTimeStat.jsonV2ParseTime.tv_sec != 0)      || (lastTimeStat.jsonV2ParseTime.tv_nsec != 0);
  bool lastMongoBackendTime     = (lastTimeStat.mongoBackendTime.tv_sec != 0)     || (lastTimeStat.mongoBackendTime.tv_nsec != 0);
  bool lastMongoReadWaitTime    = (lastTimeStat.mongoReadWaitTime.tv_sec != 0)    || (lastTimeStat.mongoReadWaitTime.tv_nsec != 0);
  bool lastMongoWriteWaitTime   = (lastTimeStat.mongoWriteWaitTime.tv_sec != 0)   || (lastTimeStat.mongoWriteWaitTime.tv_nsec != 0);
  bool lastMongoCommandWaitTime = (lastTimeStat.mongoCommandWaitTime.tv_sec != 0) || (lastTimeStat.mongoCommandWaitTime.tv_nsec != 0);
  bool lastExprBasicCtxBldTime  = (lastTimeStat.exprBasicCtxBldTime.tv_sec != 0)  || (lastTimeStat.exprBasicCtxBldTime.tv_nsec != 0);
  bool lastExprBasicEvalTime    = (lastTimeStat.exprBasicEvalTime.tv_sec != 0)    || (lastTimeStat.exprBasicEvalTime.tv_nsec != 0);
  bool lastExprJexlCtxBldTime   = (lastTimeStat.exprJexlCtxBldTime.tv_sec != 0)   || (lastTimeStat.exprJexlCtxBldTime.tv_nsec != 0);
  bool lastExprJexlEvalTime     = (lastTimeStat.exprJexlEvalTime.tv_sec != 0)     || (lastTimeStat.exprJexlEvalTime.tv_nsec != 0);
  bool lastRenderTime           = (lastTimeStat.renderTime.tv_sec != 0)           || (lastTimeStat.renderTime.tv_nsec != 0);
  bool lastReqTime              = (lastTimeStat.reqTime.tv_sec != 0)              || (lastTimeStat.reqTime.tv_nsec != 0);

  bool last = lastJsonV1ParseTime || lastJsonV2ParseTime || lastMongoBackendTime || lastRenderTime || lastReqTime;
  bool acc  = accJsonV1ParseTime || accJsonV2ParseTime || accMongoBackendTime || accRenderTime || accReqTime;

  if (!acc && !last)
  {
    timeStatSemGive(__FUNCTION__, "no stats to report");
    return "{}";
  }

  JsonObjectHelper jh;

  if (acc)
  {
    JsonObjectHelper accJh;

    if (accJsonV1ParseTime)      accJh.addNumber("jsonV1Parse",      timeSpecToFloat(accTimeStat.jsonV1ParseTime));
    if (accJsonV2ParseTime)      accJh.addNumber("jsonV2Parse",      timeSpecToFloat(accTimeStat.jsonV2ParseTime));
    if (accMongoBackendTime)     accJh.addNumber("mongoBackend",     timeSpecToFloat(accTimeStat.mongoBackendTime));
    if (accMongoReadWaitTime)    accJh.addNumber("mongoReadWait",    timeSpecToFloat(accTimeStat.mongoReadWaitTime));
    if (accMongoWriteWaitTime)   accJh.addNumber("mongoWriteWait",   timeSpecToFloat(accTimeStat.mongoWriteWaitTime));
    if (accMongoCommandWaitTime) accJh.addNumber("mongoCommandWait", timeSpecToFloat(accTimeStat.mongoCommandWaitTime));
    if (accExprBasicCtxBldTime)  accJh.addNumber("exprBasicCtxBld",  timeSpecToFloat(accTimeStat.exprBasicCtxBldTime));
    if (accExprBasicEvalTime)    accJh.addNumber("exprBasicEval",    timeSpecToFloat(accTimeStat.exprBasicEvalTime));
    if (accExprJexlCtxBldTime)   accJh.addNumber("exprJexlCtxBld",   timeSpecToFloat(accTimeStat.exprJexlCtxBldTime));
    if (accExprJexlEvalTime)     accJh.addNumber("exprJexlEval",     timeSpecToFloat(accTimeStat.exprJexlEvalTime));
    if (accRenderTime)           accJh.addNumber("render",           timeSpecToFloat(accTimeStat.renderTime));
    if (accReqTime)              accJh.addNumber("total",            timeSpecToFloat(accTimeStat.reqTime));

    jh.addRaw("accumulated", accJh.str());
  }
  if (last)
  {
    JsonObjectHelper lastJh;

    if (lastJsonV1ParseTime)      lastJh.addNumber("jsonV1Parse",      timeSpecToFloat(lastTimeStat.jsonV1ParseTime));
    if (lastJsonV2ParseTime)      lastJh.addNumber("jsonV2Parse",      timeSpecToFloat(lastTimeStat.jsonV2ParseTime));
    if (lastMongoBackendTime)     lastJh.addNumber("mongoBackend",     timeSpecToFloat(lastTimeStat.mongoBackendTime));
    if (lastMongoReadWaitTime)    lastJh.addNumber("mongoReadWait",    timeSpecToFloat(lastTimeStat.mongoReadWaitTime));
    if (lastMongoWriteWaitTime)   lastJh.addNumber("mongoWriteWait",   timeSpecToFloat(lastTimeStat.mongoWriteWaitTime));
    if (lastMongoCommandWaitTime) lastJh.addNumber("mongoCommandWait", timeSpecToFloat(lastTimeStat.mongoCommandWaitTime));
    if (lastExprBasicCtxBldTime)  lastJh.addNumber("exprBasicCtxBld",  timeSpecToFloat(lastTimeStat.exprBasicCtxBldTime));
    if (lastExprBasicEvalTime)    lastJh.addNumber("exprBasicEval",    timeSpecToFloat(lastTimeStat.exprBasicEvalTime));
    if (lastExprJexlCtxBldTime)   lastJh.addNumber("exprJexlCtxBld",   timeSpecToFloat(lastTimeStat.exprJexlCtxBldTime));
    if (lastExprJexlEvalTime)     lastJh.addNumber("exprJexlEval",     timeSpecToFloat(lastTimeStat.exprJexlEvalTime));
    if (lastRenderTime)           lastJh.addNumber("render",           timeSpecToFloat(lastTimeStat.renderTime));
    if (lastReqTime)              lastJh.addNumber("total",            timeSpecToFloat(lastTimeStat.reqTime));

    jh.addRaw("last", lastJh.str());
  }

  timeStatSemGive(__FUNCTION__, "putting stats together");
  return jh.str();
}



/* ****************************************************************************
*
* timingStatisticsReset - 
*/
void timingStatisticsReset(void)
{
  memset(&accTimeStat, 0, sizeof(accTimeStat));
}



/* ****************************************************************************
*
* statisticsUpdate - 
*
* FIXME P6: No statistics for received QueryResponses (Response from Provider Application
*           after forwarding a query)
*
* There are some counter that are not updated by this function (but are rendered
* by the renderStatCounters() function). In particular:
*
* - noOfRegistrationUpdateErrors
* - noOfDiscoveryErrors
* - noOfNotificationsSent
* - noOfSimulatedNotifications (this one not in renderStatCountersU(), but in statisticsTreat() directly)
*/
void statisticsUpdate(RequestType request, MimeType inMimeType, Verb verb, const char* urlPrefix)
{
  // If statistics are not enabled at CLI, then there is no point of recording anything
  // Performance will be increased in this case
  if (!countersStatistics)
  {
    return;
  }

  if (inMimeType == JSON)
  {
    ++noOfJsonRequests;
  }
  else if (inMimeType == TEXT)
  {
    ++noOfTextRequests;
  }
  else  // inMimeType == NOMIMETYPE
  {
    ++noOfRequestsWithoutPayload;
  }

  bool requestFound = false;
  for (unsigned int ix = 0; ; ++ix)
  {
    if ((noOfRequestCounters[ix].request == request) &&
        (strncasecmp(noOfRequestCounters[ix].prefix, urlPrefix, strlen(urlPrefix))) == 0)
    {
      requestFound = true;
      switch(verb)
      {
      case GET:
        if (noOfRequestCounters[ix].getAllowed)
        {
          ++noOfRequestCounters[ix].get;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;

      case POST:
        if (noOfRequestCounters[ix].postAllowed)
        {
          ++noOfRequestCounters[ix].post;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;

      case PATCH:
        if (noOfRequestCounters[ix].patchAllowed)
        {
          ++noOfRequestCounters[ix].patch;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;

      case PUT:
        if (noOfRequestCounters[ix].putAllowed)
        {
          ++noOfRequestCounters[ix].put;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;

      case DELETE:
        if (noOfRequestCounters[ix].deleteAllowed)
        {
          ++noOfRequestCounters[ix]._delete;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;

      case OPTIONS:
        if (noOfRequestCounters[ix].optionsAllowed)
        {
          ++noOfRequestCounters[ix].options;
        }
        else
        {
          ++noOfMissedVerb;
        }
        break;
      default:
        ++noOfMissedVerb;
      }
    }

    // We know that LeakRequest is the last request type in the array, by construction
    // FIXME P7: this is weak (but it works)
    if ((requestFound) || (noOfRequestCounters[ix].request == LeakRequest))
    {
      break;
    }
  }

  // If it is not a NGSIv2 request it has to be NGSIv1 or invalid
  if (!requestFound)
  {
    ++noOfInvalidRequests;
  }
}
