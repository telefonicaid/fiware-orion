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
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "alarmMgr/alarmMgr.h"
#include "serviceRoutinesV2/versionTreat.h"     // For orionInit()
#include "mongoBackend/MongoGlobal.h"         // For orionInit()
#include "mongoBackend/dbConstants.h"         // For mongoInit()



/* ****************************************************************************
*
* Globals
*/
static Timer*          timer                = NULL;
int                    startTime            = -1;
int                    statisticsTime       = -1;
OrionExitFunction      orionExitFunction    = NULL;
static struct timeval  logStartTime;
bool                   countersStatistics   = false;
bool                   semWaitStatistics    = false;
bool                   timingStatistics     = false;
bool                   notifQueueStatistics = false;
unsigned long long     inReqPayloadMaxSize  = DEFAULT_IN_REQ_PAYLOAD_MAX_SIZE;
unsigned long long     outReqMsgMaxSize     = DEFAULT_OUT_REQ_MSG_MAX_SIZE;



/* ****************************************************************************
*
* transactionIdGet - 
*
* Unless readonly, add one to the transactionId and return it.
* If readonly - just return the current transactionId.
*/
int transactionIdGet(bool readonly)
{
  static int transactionId = 0;

  if (readonly == false)
  {
    ++transactionId;
    if (transactionId < 0)
    {
      transactionId = 1;
    }
  }

  return transactionId;
}



/* ****************************************************************************
*
* transactionIdGetAsString -
*
* Different from transactionIdGet(), this function returns the full transID,
* not only the integer counter
*/
char* transactionIdGetAsString(void)
{
  return transactionId;
}



/* ****************************************************************************
*
* transactionIdSet - set the transaction ID
*
* To ensure a unique identifier of the transaction, the startTime down to milliseconds
* of the broker is used as prefix (to almost guarantee its uniqueness among brokers)
* Furthermore, a running number is appended for the transaction.
* A 32 bit signed number is used, so its max value is 0x7FFFFFFF (2,147,483,647).
*
* If the running number overflows, a millisecond is added to the start time of the broker.
* As the running number starts from 1 again after overflow, we need this to distinguish the first transaction
* after a running number overflow from the VERY first transaction (as both will have running number 1).
* Imagine that the start time of the broker is XXXXXXXXX.123:
*
*   XXXXXXXXX.123.1  # the VERY first transaction
*   XXXXXXXXX.124.1  # the first transaction after running number overflow
*
* The whole thing is stored in the thread variable 'transactionId', supported by the
* logging library 'liblm'.
*
*/
void transactionIdSet(void)
{
  static int firstTime = true;

  transSemTake("transactionIdSet", "changing the transaction id");

  int   transaction = transactionIdGet(false);

  if ((firstTime == false) && (transaction == 1))  // Overflow - see function header for explanation
  {
    logStartTime.tv_usec += 1;
  }

  snprintf(transactionId, sizeof(transactionId), "%lu-%03d-%011d",
           logStartTime.tv_sec, (int) logStartTime.tv_usec / 1000, transaction);

  transSemGive("transactionIdSet", "changing the transaction id");

  firstTime = false;
}



/* ****************************************************************************
*
* transactionIdSet - set the transaction ID string
*
* Different from the argument-less version of this function, this one
* just sets the thread variable for transaction ID. It's pretty
* similar to correlatorIdSet()
*/
void transactionIdSet(const char* transId)
{
  strncpy(transactionId, transId, sizeof(transactionId));
}



/* ****************************************************************************
*
* correlationIdGet -
*
*/
char* correlationIdGet(void)
{
  return correlatorId;
}



/* ****************************************************************************
*
* correlatorIdSet - 
*/
void correlatorIdSet(const char* corrId)
{
  strncpy(correlatorId, corrId, sizeof(correlatorId));
}



/* ****************************************************************************
*
* orionInit - 
*/
void orionInit
(
  OrionExitFunction  exitFunction,
  const char*        version,
  SemOpType          reqPolicy,
  bool               _countersStatistics,
  bool               _semWaitStatistics,
  bool               _timingStatistics,
  bool               _notifQueueStatistics
)
{
  // Give the rest library the correct version string of this executable
  versionSet(version);

  // The function to call on fatal error
  orionExitFunction = exitFunction;

  // Initialize the semaphore used by mongoBackend
  semInit(reqPolicy, _semWaitStatistics);

  // Set timer object (singleton)
  setTimer(new Timer());

  // startTime for log library
  if (gettimeofday(&logStartTime, NULL) != 0)
  {
    fprintf(stderr, "gettimeofday: %s\n", strerror(errno));
    orionExitFunction(1, "gettimeofday error");
  }

  // Set start time and statisticsTime used by REST interface
  startTime      = logStartTime.tv_sec;
  statisticsTime = startTime;

  // Set other flags related with statistics
  countersStatistics   = _countersStatistics;
  timingStatistics     = _timingStatistics;
  notifQueueStatistics = _notifQueueStatistics;

  strncpy(transactionId, "N/A", sizeof(transactionId));
}



/*****************************************************************************
*
* getTimer -
*/
Timer* getTimer(void)
{
  return timer;
}



/*****************************************************************************
*
* setTimer -
*/
void setTimer(Timer* t)
{
  timer = t;
}



/* ****************************************************************************
*
* getCurrentTime - 
*/ 
double getCurrentTime(void)
{
  if (getTimer() == NULL)
  {
    LM_T(LmtSoftError, ("getTimer() == NULL - calling exit function for library user"));
    orionExitFunction(1, "getTimer() == NULL");
    return -1;
  }

  double t = getTimer()->getCurrentTime();

  return t;
}



/*****************************************************************************
*
* timezoneOffset -
*
* Returns the time offset corresponding to a given timezone. Only the ISO8601 timezonoe
* formats are supported (https://en.wikipedia.org/wiki/ISO_8601#Time_zone_designators):
*
* <time>Z
* <time>±hh:mm
* <time>±hhmm
* <time>±hh
*
* The value -1 is used as "wrong timezone" (note that no timezone corresponds to an
* offset of just one negative second)
*
*/
static int timezoneOffset(const char* tz)
{
  // Trying the <time>Z format
  if (strcmp(tz, "Z") == 0)
  {
    return 0;
  }

  // All other cases start by + or -
  if ((tz[0] != '+') && (tz[0] != '-'))
  {
    return -1;
  }

  int sign   = (tz[0] == '+')? 1 : -1;
  int offset = -1;
  int h;
  int m;

  if (sscanf(tz + 1, "%2d:%2d", &h, &m) == 2)      // Trying the <time>±hh:mm format
  {
    offset = h * 60 * 60 + m * 60;
  }
  else if (sscanf(tz + 1, "%2d%2d", &h, &m) == 2)  // Trying the <time>±hhmm format
  {
    offset = h * 60 * 60 + m * 60;
  }
  else if (sscanf(tz + 1, "%2d", &h) == 1)         // Trying the <time>±hh format
  {
    offset = h * 60 * 60;
  }

  if (offset == -1)
  {
    // invalid timezone
    return -1;
  }

  return sign * offset;
}



/*****************************************************************************
*
* isLeapYear -
*
* This code will check, if the given year is a leap year or not.
*
*/
bool isLeapYear(int year)
{
  // ref: https://www.geeksforgeeks.org/program-check-given-year-leap-year/
  if (year % 400 == 0)
  {
    return true;
  }
  if ((year % 4 == 0) && (year % 100 != 0))
  {
    return true;
  }
  return false;
}



/*****************************************************************************
*
* daysInMonth -
*
* This code will check correct number of days in given month.
*
*/
int daysInMonth(int year, int month)
{
  if (month == 1) //february
  {
    return isLeapYear(year) ? 29 : 28;
  }
  // for April, June, September, November
  if (month == 3 || month == 5 || month == 8 || month == 10)
  {
    return 30;
  }
  // For all other months (Jan, March, May, July, Aug, October, December)
  return 31;
}



/*****************************************************************************
*
* parse8601Time -
**
* Based in http://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
*
*/
double parse8601Time(const std::string& ss)
{
  int    y = 0;
  int    M = 0;
  int    d = 0;
  int    h = 0;
  int    m = 0;
  float  s = 0;
  char   tz[10];

  // Length check, to avoid buffer overflow in tz[]. Calculation is as follows:
  //
  //  5 (year with "-") + 3 * 2 (day and month with "-" or "T")
  //  3 * 3 (hour/minute/second with ":" or ".") + 3 (miliseconds) + 6 (worst case timezone: "+01:00" = 29
  if (ss.length() > 29)
  {
    return -1;
  }

  // The following 'for' loop is implemented to handle a specific datetime case where the datetime string
  // is '2016-04-05T14:10:0x.00Z'. This particular case is being incorrectly PASS through the
  // sscanf() function i.e. used next to this 'for' loop.
  for (int i = 0; ss[i] != '\0'; i++)
  {
    char c = ss[i];
    if (isalpha(c) && c != 'T' && c != 'Z')
    {
      return -1;
    }
  }

  // According to https://en.wikipedia.org/wiki/ISO_8601#Times, the following formats have to be supported
  //
  // hh:mm:ss.sss or  hhmmss.sss
  // hh:mm:ss     or  hhmmss
  // hh:mm        or  hhmm
  // hh
  //
  // With regards the first case (hh:mm:ss.sss or hhmmss.sss) note that by the way sscanf() works for the %f
  // formater, this will work not only with .000, but also with .0, .00, .0000, etc. This is fine with ISO8601
  // which states that "There is no limit on the number of decimal places for the decimal fraction".

  // Default timezone is Z, sscanf will override it if an explicit timezone is provided
  snprintf(tz, sizeof(tz), "%s", "Z");

  bool validDate = ((sscanf(ss.c_str(), "%4d-%2d-%2dT%2d:%2d:%f%s", &y, &M, &d, &h, &m, &s, tz) >= 6)  ||  // Trying hh:mm:ss.sss or hh:mm:ss
                    (sscanf(ss.c_str(), "%4d-%2d-%2dT%2d%2d%f%s", &y, &M, &d, &h, &m, &s, tz) >= 6)    ||  // Trying hhmmss.sss or hhmmss
                    (sscanf(ss.c_str(), "%4d-%2d-%2dT%2d:%2d%s", &y, &M, &d, &h, &m, tz) >= 5)         ||  // Trying hh:mm
                    (sscanf(ss.c_str(), "%4d-%2d-%2dT%2d%2d%s", &y, &M, &d, &h, &m, tz) >= 5)          ||  // Trying hhmm
                    (sscanf(ss.c_str(), "%4d-%2d-%2dT%2d%s", &y, &M, &d, &h, tz) >= 4)                 ||  // Trying hh
                    (sscanf(ss.c_str(), "%4d-%2d-%2d%s", &y, &M, &d, tz) == 3));                           // Trying just date (in this case tz is not allowed)

  if (!validDate)
  {
    return -1;
  }

  int offset = timezoneOffset(tz);
  if (offset == -1)
  {
    return -1;
  }

  // Note that at the present moment we are not doing anything with milliseconds, but
  // in the future we could use that to increase time resolution (however, not as part
  // of the tm struct)

  struct tm time;
  time.tm_year = y - 1900; // Year since 1900
  time.tm_mon  = M - 1;    // 0-11
  time.tm_mday = d;        // 1-31
  time.tm_hour = h;        // 0-23
  time.tm_min  = m;        // 0-59
  time.tm_sec  = (int) s;  // 0-61 (0-60 in C++11)

  const int minYear = 0;
  const int minMonth = 0;
  const int maxMonth = 11;
  const int minDay = 1;
  const int maxDay = daysInMonth(y, time.tm_mon);
  const int minHour = 0;
  const int maxHour = 23;
  const int minMinute = 0;
  const int maxMinute = 59;
  const int minSecond = 0;
  const int maxSecond = 59;

  if (time.tm_year < minYear || time.tm_mon < minMonth || time.tm_mon > maxMonth || time.tm_mday < minDay ||
      time.tm_mday > maxDay || time.tm_hour < minHour || time.tm_hour > maxHour || time.tm_min < minMinute ||
      time.tm_min > maxMinute || time.tm_sec < minSecond || time.tm_sec > maxSecond)
  {
    return -1;
  }

  int64_t  totalSecs  = timegm(&time) - offset;
  float    millis     = s - (int) s;
  double   timestamp  = totalSecs;

  timestamp += millis;  // Must be done in two lines:  timestamp = totalSecs + millis fails ...

  return timestamp;
}



/* ****************************************************************************
*
* orderCoordsForBox
*
* It return false in the case of a 'degenerate' box
*
*/
bool orderCoordsForBox(double* minLat, double* maxLat, double* minLon, double* maxLon, double lat1, double lat2, double lon1, double lon2)
{
  if ((lat1 == lat2) || (lon1 == lon2))
  {
    return false;
  }

  *minLat = (lat1 < lat2)? lat1 : lat2;
  *maxLat = (lat1 > lat2)? lat1 : lat2;
  *minLon = (lon1 < lon2)? lon1 : lon2;
  *maxLon = (lon1 > lon2)? lon1 : lon2;

  return true;
}
