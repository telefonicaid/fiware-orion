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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/

#include <time.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "serviceRoutines/versionTreat.h"     // For orionInit()
#include "mongoBackend/MongoGlobal.h"         // For orionInit()
#include "ngsiNotify/onTimeIntervalThread.h"  // For orionInit()

/* ****************************************************************************
*
* Globals
*/
static Timer*     timer             = NULL;
int               startTime         = -1;
int               statisticsTime    = -1;
OrionExitFunction orionExitFunction = NULL;



/* ****************************************************************************
*
* orionInit - 
*/
void orionInit(OrionExitFunction exitFunction, const char* version, bool ngsi9Only)
{
  // Give the rest library the correct version string of this executable
  versionSet(version);

  // The function to call on fatal error
  orionExitFunction = exitFunction;

  /* Initialize the semaphore used by mongoBackend */
  semInit();

  /* Set timer object (singleton) */
  setTimer(new Timer());

  /* Set notifier object (singleton) */
  setNotifier(new Notifier());

  /* Set start time */
  startTime      = getCurrentTime();
  statisticsTime = startTime;

  /* Launch threads corresponding to ONTIMEINTERVAL subscriptions in the database (unless ngsi9 only mode) */
  if (!ngsi9Only)
    recoverOntimeIntervalThreads();
  else
    LM_F(("Running in NGSI9 only mode"));
}



/* ****************************************************************************
*
* isTrue - 
*/
bool isTrue(const std::string& s)
{
  if (strcasecmp(s.c_str(), "true") == 0)
    return true;
  if (strcasecmp(s.c_str(), "yes") == 0)
    return true;

  return false;
}



/* ****************************************************************************
*
* isFalse - 
*/
bool isFalse(const std::string& s)
{
  if (strcasecmp(s.c_str(), "false") == 0)
    return true;
  if (strcasecmp(s.c_str(), "no") == 0)
    return true;

  return false;
}


/*****************************************************************************
*
* getTimer -
*/
Timer* getTimer() {
    return timer;
}

/*****************************************************************************
*
* setTimer -
*/
void setTimer(Timer* t) {
    timer = t;
}

/* ****************************************************************************
*
* getCurrentTime - 
*/ 
int getCurrentTime(void)
{
  if (getTimer() == NULL)
  {
    LM_E(("getTimer() == NULL - calling exit function for library user"));
    orionExitFunction(1, "getTimer() == NULL");
    return -1;
  }

  return getTimer()->getCurrentTime();
}

/* ****************************************************************************
*
* toSeconds -
*/
int toSeconds(int value, char what, bool dayPart)
{
  if (dayPart == true)
  {
    if (what == 'Y')
      return 365 * 24 * 3600 * value;
    else if (what == 'M')
      return 30 * 24 * 3600 * value;
    else if (what == 'W')
      return 7 * 24 * 3600 * value;
    else if (what == 'D')
      return 24 * 3600 * value;
  }
  else
  {
    if (what == 'H')
      return 3600 * value;
    else if (what == 'M')
      return 60 * value;
    else if (what == 'S')
      return value;
  }

  LM_RE(-1, ("ERROR in duration string!"));
}

/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
int parse8601(std::string s) {

    char* duration = strdup(s.c_str());
    char* toFree   = duration;
    char* start;
    bool  dayPart = true;

    if (*duration != 'P')
    {
      free(toFree);
      return -1;
    }

    ++duration;
    start = duration;

    int accumulated = 0;
    while (*duration != 0)
    {
      if (isdigit(*duration))
        ++duration;
      else if ((dayPart == true) && ((*duration == 'Y') || (*duration == 'M') || (*duration == 'D') || (*duration == 'W')))
      {
        char what = *duration;

        *duration = 0;
        int value = atoi(start);

        accumulated += toSeconds(value, what, dayPart);
        ++duration;
        start = duration;
      }
      else if ((dayPart == true) && (*duration == 'T'))
      {
        dayPart = false;
        ++duration;
        start = duration;
      }
      else if ((dayPart == false) && ((*duration == 'H') || (*duration == 'M') || (*duration == 'S')))
      {
        char what = *duration;

        *duration = 0;
        int value = atoi(start);

        accumulated += toSeconds(value, what, dayPart);
        ++duration;
        start = duration;
      }
      else
      {
         free(toFree);
         return -1; // ParseError
      }
    }

    free(toFree);
    return accumulated;
}
