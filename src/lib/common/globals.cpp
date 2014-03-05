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
#include <string>

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
void orionInit(OrionExitFunction exitFunction, const char* version)
{
  // Give the rest library the correct version string of this executable
  versionSet(version);

  // The function to call on fatal error
  orionExitFunction = exitFunction;

  /* Initialize the semaphore used by mongoBackend */
  semInit();

  /* Set timer object (singleton) */
  setTimer(new Timer());

  /* Set start time */
  startTime      = getCurrentTime();
  statisticsTime = startTime;
}



/* ****************************************************************************
*
* isTrue - 
*/
bool isTrue(const std::string& s)
{
  if ((s == "true") || (s == "1"))
    return true;

  return false;
}



/* ****************************************************************************
*
* isFalse - 
*/
bool isFalse(const std::string& s)
{
  if ((s == "false") || (s == "0"))
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
long long toSeconds(int value, char what, bool dayPart)
{
  long long result = -1;

  if (dayPart == true)
  {
    if (what == 'Y')
      result = 365L * 24 * 3600 * value;
    else if (what == 'M')
      result = 30L * 24 * 3600 * value;
    else if (what == 'W')
      result = 7L * 24 * 3600 * value;
    else if (what == 'D')
      result = 24L * 3600 * value;
  }
  else
  {
    if (what == 'H')
      result = 3600L * value;
    else if (what == 'M')
      result = 60L * value;
    else if (what == 'S')
      result = value;
  }

  if (result == -1)
    LM_E(("ERROR in duration string!"));

  return result;
}

/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
long long parse8601(std::string s)
{
    if (s == "")
        return 0;

    char*      duration    = strdup(s.c_str());
    char*      toFree      = duration;
    bool       dayPart     = true;
    long long  accumulated = 0;
    char*      start;

    if (*duration != 'P')
    {
      free(toFree);
      return -1;
    }

    ++duration;
    start = duration;

    while (*duration != 0)
    {
      if (isdigit(*duration))
        ++duration;
      else if ((dayPart == true) && ((*duration == 'Y') || (*duration == 'M') || (*duration == 'D') || (*duration == 'W')))
      {
        char what = *duration;

        *duration = 0;
        int value = atoi(start);

        if ((value == 0) && (*start != '0'))
        {
           free(toFree);
           LM_RE(-1, ("parse error for '%s'", start));
        }

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
