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
  //                                                      GET    POST   PATCH  PUT    DELET  OPT
  {EntryPointsRequest,            -1, -1, -1, -1, -1, -1, true,  false, false, false ,false, true},
  {EntitiesRequest,               -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {EntityRequest,                 -1, -1, -1, -1, -1, -1, true,  true,  true,  true,  true,  true},
  {EntityAttributeRequest,        -1, -1, -1, -1, -1, -1, true,  false, false, true,  true,  true},
  {EntityAttributeValueRequest,   -1, -1, -1, -1, -1, -1, true,  false, false, true,  false, true},
  {EntityAllTypesRequest,         -1, -1, -1, -1, -1, -1, true,  false, false, false, false, true},
  {EntityTypes,                   -1, -1, -1, -1, -1, -1, true,  false, false, false, false, true},
  {SubscriptionsRequest,          -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {IndividualSubscriptionRequest, -1, -1, -1, -1, -1, -1, true,  false, true,  false, true,  true},
  {RegistrationsRequest,          -1, -1, -1, -1, -1, -1, true,  true,  false, false, false, true},
  {RegistrationRequest,           -1, -1, -1, -1, -1, -1, true,  false, true,  false, true,  true},
  {BatchQueryRequest,             -1, -1, -1, -1, -1, -1, false, true,  false, false, false, true},
  {BatchUpdateRequest,            -1, -1, -1, -1, -1, -1, false, true,  false, false, false, true},
  // FIXME: NotifyContext is shared for v1 and v2, both use postNotifyContext(). Weird...
  {NotifyContext,                 -1, -1, -1, -1, -1, -1, false, true,  false, false, false, false},

  {LogTraceRequest,               -1, -1, -1, -1, -1, -1, true,  false, false, true,  true,  false},
  {StatisticsRequest,             -1, -1, -1, -1, -1, -1, true,  false, false, false, true,  false},
  {VersionRequest,                -1, -1, -1, -1, -1, -1, true,  false, false, false, false, true},
  {LogLevelRequest,               -1, -1, -1, -1, -1, -1, true,  false, false, true,  false, false},
  {SemStateRequest,               -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  {MetricsRequest,                -1, -1, -1, -1, -1, -1, true,  false, false, false, true,  false},
  {ExitRequest,                   -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false},
  {LeakRequest,                   -1, -1, -1, -1, -1, -1, true,  false, false, false, false, false}
};

// Special
int noOfLegacyNgsiv1Requests     = -1;
int noOfInvalidRequests          = -1;
int noOfSimulatedNotifications   = -1;
int noOfRegistrationUpdateErrors = -1;
int noOfDiscoveryErrors          = -1;

int noOfSubCacheEntries          = -1;
int noOfSubCacheLookups          = -1;
int noOfSubCacheRemovals         = -1;
int noOfSubCacheRemovalFailures  = -1;



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
  bool accRenderTime           = (accTimeStat.renderTime.tv_sec != 0)             || (accTimeStat.renderTime.tv_nsec != 0);
  bool accReqTime              = (accTimeStat.reqTime.tv_sec != 0)                || (accTimeStat.reqTime.tv_nsec != 0);

  bool lastJsonV1ParseTime      = (lastTimeStat.jsonV1ParseTime.tv_sec != 0)      || (lastTimeStat.jsonV1ParseTime.tv_nsec != 0);
  bool lastJsonV2ParseTime      = (lastTimeStat.jsonV2ParseTime.tv_sec != 0)      || (lastTimeStat.jsonV2ParseTime.tv_nsec != 0);
  bool lastMongoBackendTime     = (lastTimeStat.mongoBackendTime.tv_sec != 0)     || (lastTimeStat.mongoBackendTime.tv_nsec != 0);
  bool lastMongoReadWaitTime    = (lastTimeStat.mongoReadWaitTime.tv_sec != 0)    || (lastTimeStat.mongoReadWaitTime.tv_nsec != 0);
  bool lastMongoWriteWaitTime   = (lastTimeStat.mongoWriteWaitTime.tv_sec != 0)   || (lastTimeStat.mongoWriteWaitTime.tv_nsec != 0);
  bool lastMongoCommandWaitTime = (lastTimeStat.mongoCommandWaitTime.tv_sec != 0) || (lastTimeStat.mongoCommandWaitTime.tv_nsec != 0);
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

bool isLegacyNgsiv1(RequestType request)
{
  switch(request)
  {
  case AllContextEntities:
  case AllEntitiesWithTypeAndId:
  case AttributesForEntityType:
  case ContextEntitiesByEntityId:
  case ContextEntitiesByEntityIdAndType:
  case ContextEntityAttributes:
  case ContextEntityTypeAttribute:
  case ContextEntityTypeAttributeContainer:
  case ContextEntityTypes:
  case DiscoverContextAvailability:
  case EntityByIdAttributeByName:
  case EntityByIdAttributeByNameIdAndType:
  case EntityTypes:
  case IndividualContextEntity:
  case IndividualContextEntityAttribute:
  case IndividualContextEntityAttributes:
  case IndividualContextEntityAttributeWithTypeAndId:
  case Ngsi10ContextEntityTypes:
  case Ngsi10ContextEntityTypesAttribute:
  case Ngsi10ContextEntityTypesAttributeContainer:
  case Ngsi10SubscriptionsConvOp:
  //case NotifyContext:  //FIXME: this is also used for v2. Weird...
  case QueryContext:
  case RegisterContext:
  case SubscribeContext:
  case UnsubscribeContext:
  case UpdateContext:
  case UpdateContextSubscription:
    return true;
  default:
    return false;
  }
}


/* ****************************************************************************
*
* statisticsUpdate - 
*
* FIXME P6: No statistics for received QueryResponses (Response from Provider Application
*           after forwarding a query)
*/
void statisticsUpdate(RequestType request, MimeType inMimeType)
{
  // TBD: look for calls to statisticsUpdate
  // Take into acccount NotifyContextSent
  // Take into account countersStatistics for early return

  if (inMimeType == JSON)
  {
    ++noOfJsonRequests;
  }
  else if (inMimeType == NOMIMETYPE)
  {
    // FIXME P4: Include this counter in the statistics (Issue #1400)
    ++noOfRequestsWithoutPayload;
  }

  /*switch (request)
  {
  // v2
  case EntryPointsRequest:  ++noOfEntryPoint; break;
  case EntitiesRequest: ++noOfEntitiesGet; break;
  case EntityRequest: ++noOfEntityGet; break;
  case EntityAttributeRequest: ++noOfArrributeGet; break;
  case EntityAttributeValueRequest: ++noOfAttributeValueGet; break;
  case EntityAllTypesRequest: ++noOfEntityTypesGet; break;
  case EntityTypeRequest: ++noOfEntityTypeGet; break;
  case SubscriptionsRequest: ++noOfSubscriptionsGet; break;
  case IndividualSubscriptionRequest: ++noOfSubscriptionsGet; break;
  case RegistrationsRequest: ++noOfRegistrationsGet; break;
  case RegistrationRequest: ++noOfRegistrationGet; break;

  // legacy ngsiv1
  case AllContextEntities:
  case AllEntitiesWithTypeAndId:
  case AttributesForEntityType:
  case ContextEntitiesByEntityId:
  case ContextEntitiesByEntityId:
  case ContextEntitiesByEntityIdAndType:
  case ContextEntityAttributes:
  case ContextEntityAttributes:
  case ContextEntityTypeAttribute:
  case ContextEntityTypeAttribute:
  case ContextEntityTypeAttributeContainer:
  case ContextEntityTypeAttributeContainer:
  case ContextEntityTypes:
  case ContextEntityTypes:
  case EntityByIdAttributeByName:
  case EntityByIdAttributeByName:
  case EntityByIdAttributeByNameIdAndType:
  case EntityTypes:
  case IndividualContextEntity:
  case IndividualContextEntity:
  case IndividualContextEntityAttribute:
  case IndividualContextEntityAttribute:
  case IndividualContextEntityAttributes:
  case IndividualContextEntityAttributes:
  case IndividualContextEntityAttributeWithTypeAndId:
  case Ngsi10ContextEntityTypes:
  case Ngsi10ContextEntityTypes:
  case Ngsi10ContextEntityTypesAttribute:
  case Ngsi10ContextEntityTypesAttribute:
  case Ngsi10ContextEntityTypesAttributeContainer:
  case Ngsi10ContextEntityTypesAttributeContainer:
    ++noOfLegacyNgsiv1Requests;
    break;

  // Special
  case LogTraceRequest:
  case StatisticsRequest:
  case VersionRequest:
  case LogLevelRequest:
  case SemStateRequest:
  case MetricsRequest:
  case ExitRequest:
  case LeakRequest:


  case NoRequest:                                        break;
  case RegisterContext:                                  ++noOfRegistrations; break;
  case DiscoverContextAvailability:                      ++noOfDiscoveries; break;

  case QueryContext:                                     ++noOfQueries; break;
  case SubscribeContext:                                 ++noOfSubscriptions; break;
  case UpdateContextSubscription:                        ++noOfSubscriptionUpdates; break;
  case UnsubscribeContext:                               ++noOfUnsubscriptions; break;
  case NotifyContext:                                    ++noOfNotificationsReceived; break;
  case NotifyContextSent:                                ++noOfNotificationsSent; break;
  case UpdateContext:                                    ++noOfUpdates; break;
  case RtQueryContextResponse:                           ++noOfQueryContextResponses; break;
  case RtUpdateContextResponse:                          ++noOfUpdateContextResponses; break;

  case ContextEntitiesByEntityId:                        ++noOfContextEntitiesByEntityId; break;
  case ContextEntityAttributes:                          ++noOfContextEntityAttributes; break;
  case EntityByIdAttributeByName:                        ++noOfEntityByIdAttributeByName; break;
  case ContextEntityTypes:                               ++noOfContextEntityTypes; break;
  case ContextEntityTypeAttributeContainer:              ++noOfContextEntityTypeAttributeContainer; break;
  case ContextEntityTypeAttribute:                       ++noOfContextEntityTypeAttribute; break;
  case IndividualContextEntity:                          ++noOfIndividualContextEntity; break;
  case IndividualContextEntityAttributes:                ++noOfIndividualContextEntityAttributes; break;
  case AttributeValueInstance:                           ++noOfAttributeValueInstance; break;
  case IndividualContextEntityAttribute:                 ++noOfIndividualContextEntityAttribute; break;

  case UpdateContextElement:                             ++noOfUpdateContextElement; break;
  case AppendContextElement:                             ++noOfAppendContextElement; break;
  case UpdateContextAttribute:                           ++noOfUpdateContextAttribute; break;
  case Ngsi10ContextEntityTypes:                         ++noOfNgsi10ContextEntityTypes; break;
  case Ngsi10ContextEntityTypesAttributeContainer:       ++noOfNgsi10ContextEntityTypesAttributeContainer; break;
  case Ngsi10ContextEntityTypesAttribute:                ++noOfNgsi10ContextEntityTypesAttribute; break;
  case Ngsi10SubscriptionsConvOp:                        ++noOfNgsi10SubscriptionsConvOp; break;

  case AllContextEntities:                               ++noOfAllContextEntitiesRequests; break;
  case AllEntitiesWithTypeAndId:                         ++noOfAllEntitiesWithTypeAndIdRequests; break;
  case IndividualContextEntityAttributeWithTypeAndId:    ++noOfIndividualContextEntityAttributeWithTypeAndId; break;
  case AttributeValueInstanceWithTypeAndId:              ++noOfAttributeValueInstanceWithTypeAndId; break;
  case ContextEntitiesByEntityIdAndType:                 ++noOfContextEntitiesByEntityIdAndType; break;
  case EntityByIdAttributeByNameIdAndType:               ++noOfEntityByIdAttributeByNameIdAndType; break;

  case LogTraceRequest:                                  ++noOfLogTraceRequests; break;
  case LogLevelRequest:                                  ++noOfLogLevelRequests; break;
  case SemStateRequest:                                  ++noOfSemStateRequests; break;
  case MetricsRequest:                                   ++noOfMetricsRequests; break;
  case VersionRequest:                                   ++noOfVersionRequests; break;
  case ExitRequest:                                      ++noOfExitRequests; break;
  case LeakRequest:                                      ++noOfLeakRequests; break;
  case StatisticsRequest:                                ++noOfStatisticsRequests; break;

  case InvalidRequest:                                   ++noOfInvalidRequests; break;
  case RegisterResponse:                                 ++noOfRegisterResponses; break;

  case RtUnsubscribeContextResponse:                     ++noOfRtUnsubscribeContextResponse; break;
  case RtSubscribeResponse:                              ++noOfRtSubscribeResponse; break;
  case RtSubscribeError:                                 ++noOfRtSubscribeError; break;
  case RtContextElementResponse:                         ++noOfContextElementResponse; break;
  case RtContextAttributeResponse:                       ++noOfContextAttributeResponse; break;
  case EntityTypes:                                      ++noOfEntityTypesRequest; break;
  case AttributesForEntityType:                          ++noOfAttributesForEntityTypeRequest; break;
  case RtEntityTypesResponse:                            ++noOfEntityTypesResponse; break;
  case RtAttributesForEntityTypeResponse:                ++noOfAttributesForEntityTypeResponse; break;

  case EntitiesRequest:                                  ++noOfEntitiesRequests; break;
  case EntitiesResponse:                                 ++noOfEntitiesResponses; break;
  case EntryPointsRequest:                               ++noOfEntryPointsRequests; break;
  case EntryPointsResponse:                              ++noOfEntryPointsResponses; break;
  case EntityRequest:                                    ++noOfEntityRequests; break;
  case EntityResponse:                                   ++noOfEntityResponses; break;
  case EntityAttributeRequest:                           ++noOfEntityAttributeRequests; break;
  case EntityAttributeResponse:                          ++noOfEntityAttributeResponses; break;
  case EntityAttributeValueRequest:                      ++noOfEntityAttributeValueRequests; break;
  case EntityAttributeValueResponse:                     ++noOfEntityAttributeValueResponses; break;

  case PostEntity:                                       ++noOfPostEntity; break;
  case PostAttributes:                                   ++noOfPostAttributes; break;
  case DeleteEntity:                                     ++noOfDeleteEntity; break;

  case EntityTypeRequest:                                ++noOfEntityTypeRequest; break;
  case EntityAllTypesRequest:                            ++noOfEntityAllTypesRequest; break;
  case SubscriptionsRequest:                             ++noOfSubscriptionsRequest; break;
  case IndividualSubscriptionRequest:                    ++noOfIndividualSubscriptionRequest; break;
  case BatchQueryRequest:                                ++noOfBatchQueryRequest; break;
  case BatchUpdateRequest:                               ++noOfBatchUpdateRequest; break;

  case RegistrationRequest:                              ++noOfRegistrationRequest; break;
  case RegistrationsRequest:                             ++noOfRegistrationsRequest; break;

  }*/
}
