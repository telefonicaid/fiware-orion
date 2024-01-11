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

#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "alarmMgr/alarmMgr.h"
#include "serviceRoutines/versionTreat.h"     // For orionInit()
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
bool                   checkIdv1            = false;


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
* correlatorIdSet - 
*/
void correlatorIdSet(const char* corrId)
{
  strncpy(correlatorId, corrId, sizeof(correlatorId) - 1);
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
  bool               _notifQueueStatistics,
  bool               _checkIdv1
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

  strncpy(transactionId, "N/A", sizeof(transactionId) - 1);

  checkIdv1 = _checkIdv1;
}



/* ****************************************************************************
*
* isTrue - 
*/
bool isTrue(const std::string& s)
{
  if ((s == "true") || (s == "1"))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* isFalse - 
*/
bool isFalse(const std::string& s)
{
  if ((s == "false") || (s == "0"))
  {
    return true;
  }

  return false;
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
    LM_W(("getTimer() == NULL - calling exit function for library user"));
    orionExitFunction(1, "getTimer() == NULL");
    return -1;
  }

  return getTimer()->getCurrentTime();
}



/* ****************************************************************************
*
* toSeconds -
*/
int64_t toSeconds(int value, char what, bool dayPart)
{
  int64_t result = -1;

  if (dayPart == true)
  {
    if (what == 'Y')
    {
      result = 365L * 24 * 3600 * value;
    }
    else if (what == 'M')
    {
      result = 30L * 24 * 3600 * value;
    }
    else if (what == 'W')
    {
      result = 7L * 24 * 3600 * value;
    }
    else if (what == 'D')
    {
      result = 24L * 3600 * value;
    }
  }
  else
  {
    if (what == 'H')
    {
      result = 3600L * value;
    }
    else if (what == 'M')
    {
      result = 60L * value;
    }
    else if (what == 'S')
    {
      result = value;
    }
  }

  if (result == -1)
  {
    alarmMgr.badInput(orionldState.clientIp, "ERROR in duration string");
  }

  return result;
}



/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
int64_t parse8601(const std::string& s)
{
  if (s == "")
  {
    return -1;
  }

  char*      duration    = strdup(s.c_str());
  char*      toFree      = duration;
  bool       dayPart     = true;
  int64_t    accumulated = 0;
  char*      start;

  if (*duration != 'P')
  {
    free(toFree);
    return -1;
  }

  ++duration;
  start = duration;

  if (*duration == 0)
  {
    free(toFree);
    return -1;
  }

  bool digitsPending = false;

  while (*duration != 0)
  {
    if (isdigit(*duration) || (*duration == '.') || (*duration == ','))
    {
      ++duration;
      digitsPending = true;
    }
    else if ((dayPart == true) &&
             ((*duration == 'Y') || (*duration == 'M') || (*duration == 'D') || (*duration == 'W')))
    {
      char what = *duration;

      *duration = 0;
      int value = atoi(start);

      if ((value == 0) && (*start != '0'))
      {
        std::string details = std::string("parse error for duration '") + start + "'";
        alarmMgr.badInput(orionldState.clientIp, details);

        free(toFree);
        return -1;
      }

      accumulated += toSeconds(value, what, dayPart);
      digitsPending = false;
      ++duration;
      start = duration;
    }
    else if ((dayPart == true) && (*duration == 'T'))
    {
      dayPart = false;
      ++duration;
      start = duration;
      digitsPending = false;
    }
    else if ((dayPart == false) &&
             ((*duration == 'H') || (*duration == 'M') || (*duration == 'S')))
    {
      char what = *duration;
      int  value;

      *duration = 0;

      if (what == 'S')  // We support floats for the seconds, but only to round to an integer
      {
        // NOTE: here we use atof and not str2double on purpose
        float secs  = atof(start);
        value       = (int) round(secs);
      }
      else
      {
        value = atoi(start);
      }

      accumulated += toSeconds(value, what, dayPart);
      digitsPending = false;
      ++duration;
      start = duration;
    }
    else
    {
      free(toFree);
      return -1;  // ParseError
    }
  }

  free(toFree);

  if (digitsPending == true)
  {
    return -1;
  }

  return accumulated;
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
