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
int noOfJsonRequests                                     = -1;
int noOfRequestsWithoutPayload                           = -1;
int noOfRegistrations                                    = -1;
int noOfRegistrationErrors                               = -1;
int noOfRegistrationUpdates                              = -1;
int noOfRegistrationUpdateErrors                         = -1;
int noOfDiscoveries                                      = -1;
int noOfDiscoveryErrors                                  = -1;
int noOfAvailabilitySubscriptions                        = -1;
int noOfAvailabilitySubscriptionErrors                   = -1;
int noOfAvailabilityUnsubscriptions                      = -1;
int noOfAvailabilityUnsubscriptionErrors                 = -1;
int noOfAvailabilitySubscriptionUpdates                  = -1;
int noOfAvailabilitySubscriptionUpdateErrors             = -1;
int noOfAvailabilityNotificationsReceived                = -1;
int noOfAvailabilityNotificationsSent                    = -1;

int noOfQueries                                          = -1;
int noOfQueryErrors                                      = -1;
int noOfUpdates                                          = -1;
int noOfUpdateErrors                                     = -1;
int noOfSubscriptions                                    = -1;
int noOfSubscriptionErrors                               = -1;
int noOfSubscriptionUpdates                              = -1;
int noOfSubscriptionUpdateErrors                         = -1;
int noOfUnsubscriptions                                  = -1;
int noOfUnsubscriptionErrors                             = -1;
int noOfNotificationsReceived                            = -1;
int noOfNotificationsSent                                = -1;
int noOfQueryContextResponses                            = -1;
int noOfUpdateContextResponses                           = -1;

int noOfContextEntitiesByEntityId                        = -1;
int noOfContextEntityAttributes                          = -1;
int noOfEntityByIdAttributeByName                        = -1;
int noOfContextEntityTypes                               = -1;
int noOfContextEntityTypeAttributeContainer              = -1;
int noOfContextEntityTypeAttribute                       = -1;
int noOfNgsi9SubscriptionsConvOp                         = -1;

int noOfIndividualContextEntity                          = -1;
int noOfIndividualContextEntityAttributes                = -1;
int noOfIndividualContextEntityAttribute                 = -1;
int noOfAttributeValueInstance                           = -1;
int noOfNgsi10ContextEntityTypes                         = -1;
int noOfNgsi10ContextEntityTypesAttributeContainer       = -1;
int noOfNgsi10ContextEntityTypesAttribute                = -1;
int noOfNgsi10SubscriptionsConvOp                        = -1;

int noOfUpdateContextElement                             = -1;
int noOfAppendContextElement                             = -1;
int noOfUpdateContextAttribute                           = -1;

int noOfAllContextEntitiesRequests                       = -1;
int noOfAllEntitiesWithTypeAndIdRequests                 = -1;
int noOfIndividualContextEntityAttributeWithTypeAndId    = -1;
int noOfAttributeValueInstanceWithTypeAndId              = -1;
int noOfEntityByIdAttributeByNameIdAndType               = -1;

int noOfLogTraceRequests                                 = -1;
int noOfLogLevelRequests                                 = -1;
int noOfSemStateRequests                                 = -1;
int noOfMetricsRequests                                  = -1;
int noOfVersionRequests                                  = -1;
int noOfExitRequests                                     = -1;
int noOfLeakRequests                                     = -1;
int noOfStatisticsRequests                               = -1;
int noOfInvalidRequests                                  = -1;
int noOfRegisterResponses                                = -1;

int noOfRtSubscribeContextAvailabilityResponse           = -1;
int noOfRtUpdateContextAvailabilitySubscriptionResponse  = -1;
int noOfRtUnsubscribeContextAvailabilityResponse         = -1;
int noOfRtUnsubscribeContextResponse                     = -1;
int noOfRtSubscribeResponse                              = -1;
int noOfRtSubscribeError                                 = -1;
int noOfContextElementResponse                           = -1;
int noOfContextAttributeResponse                         = -1;

int noOfEntityTypesRequest                               = -1;
int noOfEntityTypesResponse                              = -1;
int noOfAttributesForEntityTypeRequest                   = -1;
int noOfAttributesForEntityTypeResponse                  = -1;
int noOfContextEntitiesByEntityIdAndType                 = -1;

int noOfEntitiesRequests                                 = -1;
int noOfEntitiesResponses                                = -1;

int noOfEntryPointsRequests                              = -1;
int noOfEntryPointsResponses                             = -1;

int noOfEntityRequests                                   = -1;
int noOfEntityResponses                                  = -1;

int noOfEntityAttributeRequests                          = -1;
int noOfEntityAttributeResponses                         = -1;

int noOfEntityAttributeValueRequests                     = -1;
int noOfEntityAttributeValueResponses                    = -1;

int noOfPostEntity                                       = -1;

int noOfPostAttributes                                   = -1;
int noOfDeleteEntity                                     = -1;
int noOfSubCacheEntries                                  = -1;
int noOfSubCacheLookups                                  = -1;
int noOfSubCacheRemovals                                 = -1;
int noOfSubCacheRemovalFailures                          = -1;
int noOfEntityTypeRequest                                = -1;
int noOfEntityAllTypesRequest                            = -1;
int noOfSubscriptionsRequest                             = -1;
int noOfIndividualSubscriptionRequest                    = -1;
int noOfSimulatedNotifications                           = -1;
int noOfBatchQueryRequest                                = -1;
int noOfBatchUpdateRequest                               = -1;
int noOfRegistrationRequest                              = -1;
int noOfRegistrationsRequest                             = -1;



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



/* ****************************************************************************
*
* statisticsUpdate - 
*
* FIXME P6: No statistics for received QueryResponses (Response from Provider Application
*           after forwarding a query)
*/
void statisticsUpdate(RequestType request, MimeType inMimeType)
{
  if (inMimeType == JSON)
  {
    ++noOfJsonRequests;
  }
  else if (inMimeType == NOMIMETYPE)
  {
    // FIXME P4: Include this counter in the statistics (Issue #1400)
    ++noOfRequestsWithoutPayload;
  }

  switch (request)
  {
  case NoRequest:                                        break;
  case RegisterContext:                                  ++noOfRegistrations; break;
  case DiscoverContextAvailability:                      ++noOfDiscoveries; break;
  case SubscribeContextAvailability:                     ++noOfAvailabilitySubscriptions; break;
  case UpdateContextAvailabilitySubscription:            ++noOfAvailabilitySubscriptionUpdates; break;
  case UnsubscribeContextAvailability:                   ++noOfAvailabilityUnsubscriptions; break;
  case NotifyContextAvailability:                        ++noOfAvailabilityNotificationsReceived; break;

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
  case Ngsi9SubscriptionsConvOp:                         ++noOfNgsi9SubscriptionsConvOp; break;

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

  case RtSubscribeContextAvailabilityResponse:           ++noOfRtSubscribeContextAvailabilityResponse; break;
  case RtUpdateContextAvailabilitySubscriptionResponse:  ++noOfRtUpdateContextAvailabilitySubscriptionResponse; break;
  case RtUnsubscribeContextAvailabilityResponse:         ++noOfRtUnsubscribeContextAvailabilityResponse; break;
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
  }
}
