#ifndef SRC_LIB_COMMON_STATISTICS_H_
#define SRC_LIB_COMMON_STATISTICS_H_

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
#include "ngsi/Request.h"
#include "common/MimeType.h"
#include "common/clockFunctions.h"
#include "rest/Verb.h"



/* ****************************************************************************
*
* TIME_STAT_RENDER_START - 
*/
#define TIME_STAT_RENDER_START()                                       \
  struct timespec renderStart;                                         \
  struct timespec renderEnd;                                           \
                                                                       \
  if (timingStatistics)                                               \
  {                                                                    \
    clock_gettime(CLOCK_REALTIME, &renderStart);                       \
  }



/* ****************************************************************************
*
* TIME_STAT_RENDER_STOP - 
*/
#define TIME_STAT_RENDER_STOP()                                                   \
  if (timingStatistics)                                                           \
  {                                                                               \
    struct timespec diff;                                                         \
    clock_gettime(CLOCK_REALTIME, &renderEnd);                                    \
    clock_difftime(&renderEnd, &renderStart, &diff);                              \
    clock_addtime(&threadLastTimeStat.renderTime, &diff);                         \
  }



/* ****************************************************************************
*
* TIMED_RENDER - 
*/
#define TIMED_RENDER(call)   \
{                            \
  TIME_STAT_RENDER_START();  \
  call;                      \
  TIME_STAT_RENDER_STOP();   \
}



/* ****************************************************************************
*
* TIME_STAT_MONGO_START - 
*/
#define TIME_STAT_MONGO_START()                                        \
  struct timespec mongoStart;                                          \
  struct timespec mongoEnd;                                            \
                                                                       \
  if (timingStatistics)                                                \
  {                                                                    \
    clock_gettime(CLOCK_REALTIME, &mongoStart);                        \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_STOP - 
*/
#define TIME_STAT_MONGO_STOP()                                                    \
  if (timingStatistics)                                                           \
  {                                                                               \
    struct timespec diff;                                                         \
    clock_gettime(CLOCK_REALTIME, &mongoEnd);                                     \
    clock_difftime(&mongoEnd, &mongoStart, &diff);                                \
    clock_addtime(&threadLastTimeStat.mongoBackendTime, &diff);                   \
  }



/* ****************************************************************************
*
* TIMED_MONGO - 
*/
#define TIMED_MONGO(call)   \
{                           \
  TIME_STAT_MONGO_START();  \
  call;                     \
  TIME_STAT_MONGO_STOP();   \
}



/* ****************************************************************************
*
* TIME_STAT_MONGO_READ_WAIT_START - 
*/
#define TIME_STAT_MONGO_READ_WAIT_START()                                     \
  struct timespec mongoReadWaitStart;                                         \
  struct timespec mongoReadWaitEnd;                                           \
                                                                              \
  if (timingStatistics)                                                       \
  {                                                                           \
    clock_gettime(CLOCK_REALTIME, &mongoReadWaitStart);                       \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_READ_WAIT_STOP - 
*/
#define TIME_STAT_MONGO_READ_WAIT_STOP()                                \
  if (timingStatistics)                                                 \
  {                                                                     \
    struct timespec diff;                                               \
    clock_gettime(CLOCK_REALTIME, &mongoReadWaitEnd);                   \
    clock_difftime(&mongoReadWaitEnd, &mongoReadWaitStart, &diff);      \
    clock_addtime(&threadLastTimeStat.mongoReadWaitTime, &diff);        \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_WRITE_WAIT_START - 
*/
#define TIME_STAT_MONGO_WRITE_WAIT_START()                                     \
  struct timespec mongoWriteWaitStart;                                         \
  struct timespec mongoWriteWaitEnd;                                           \
                                                                               \
  if (timingStatistics)                                                        \
  {                                                                            \
    clock_gettime(CLOCK_REALTIME, &mongoWriteWaitStart);                       \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_WRITE_WAIT_STOP - 
*/
#define TIME_STAT_MONGO_WRITE_WAIT_STOP()                             \
  if (timingStatistics)                                               \
  {                                                                   \
    struct timespec diff;                                             \
    clock_gettime(CLOCK_REALTIME, &mongoWriteWaitEnd);                \
    clock_difftime(&mongoWriteWaitEnd, &mongoWriteWaitStart, &diff);  \
    clock_addtime(&threadLastTimeStat.mongoWriteWaitTime, &diff);     \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_COMMAND_WAIT_START - 
*/
#define TIME_STAT_MONGO_COMMAND_WAIT_START()                                     \
  struct timespec mongoCommandWaitStart;                                         \
  struct timespec mongoCommandWaitEnd;                                           \
                                                                                 \
  if (timingStatistics)                                                          \
  {                                                                              \
    clock_gettime(CLOCK_REALTIME, &mongoCommandWaitStart);                       \
  }



/* ****************************************************************************
*
* TIME_STAT_MONGO_COMMAND_WAIT_STOP - 
*/
#define TIME_STAT_MONGO_COMMAND_WAIT_STOP()                              \
  if (timingStatistics)                                                  \
  {                                                                      \
    struct timespec diff;                                                \
    clock_gettime(CLOCK_REALTIME, &mongoCommandWaitEnd);                 \
    clock_difftime(&mongoCommandWaitEnd, &mongoCommandWaitStart, &diff); \
    clock_addtime(&threadLastTimeStat.mongoCommandWaitTime, &diff);      \
  }



/* ****************************************************************************
*
* TimeStat - 
*/
typedef struct TimeStat
{
  struct timespec  jsonV1ParseTime;
  struct timespec  jsonV2ParseTime;
  struct timespec  mongoBackendTime;
  struct timespec  mongoReadWaitTime;
  struct timespec  mongoWriteWaitTime;
  struct timespec  mongoCommandWaitTime;
  struct timespec  renderTime;
  struct timespec  reqTime;
} TimeStat;

extern TimeStat           accTimeStat;
extern TimeStat           lastTimeStat;
extern __thread TimeStat  threadLastTimeStat;


/* ****************************************************************************
*
* UrlCounter -
*/
typedef struct UrlCounter
{
  RequestType request;

  int get;
  int post;
  int patch;
  int put;
  int _delete;
  int options;

  bool getAllowed;
  bool postAllowed;
  bool patchAllowed;
  bool putAllowed;
  bool deleteAllowed;
  bool optionsAllowed;

} UrlCounter;


/* ****************************************************************************
*
* Statistic counters for NGSI REST requests
*/
// By content
extern int noOfJsonRequests;
extern int noOfTextRequests;
extern int noOfRequestsWithoutPayload;

// By url
extern UrlCounter noOfRequestCounters[];

// Special
extern int noOfVersionRequests;
extern int noOfLegacyNgsiv1Requests;
extern int noOfInvalidRequests;
extern int noOfMissedVerb;
extern int noOfRegistrationUpdateErrors;
extern int noOfDiscoveryErrors;
extern int noOfNotificationsSent;
extern int noOfSimulatedNotifications;


/* ****************************************************************************
*
* renderTimingStatistics -
*/
extern std::string renderTimingStatistics(void);



/* ****************************************************************************
*
* timingStatisticsReset - 
*/
extern void timingStatisticsReset(void);



/* ****************************************************************************
*
* statisticsUpdate - 
*/
extern void statisticsUpdate(RequestType request, MimeType inMimeType, Verb verb);

#endif  // SRC_LIB_COMMON_STATISTICS_H_
