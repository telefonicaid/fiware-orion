/*
*
* Copyright 2024 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <unistd.h>                                            // NULL
#include <string.h>                                            // strchr
#include <stdlib.h>                                            // strtod
#include <time.h>                                              // timegm, struct tm

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/stringStrip.h"                        // stringStrip
#include "orionld/common/dateTime.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// dateTimeToString -
//
char* dateTimeToString(double timestamp)
{
  return NULL;
}



// -----------------------------------------------------------------------------
//
// dateTimeFromString -
//
// Supported formats:
// - YYYY-MM-DD
// - YYYY-MM-DD T HH:mm:SS
// - YYYY-MM-DD T HH:mm:SS.mmmZ
//
double dateTimeFromString(const char* timeString)
{
  char iso8601[64];
  strncpy(iso8601, timeString, sizeof(iso8601) - 1);

  LM_T(LmtDateTime, ("In: '%s'", iso8601));

  char* date = stringStrip(iso8601);
  char* T    = strchr(date, 'T');
  char* time = NULL;

  char*  year     = date;
  char*  month    = NULL;
  char*  day      = NULL;
  char*  hours    = NULL;
  char*  minutes  = NULL;
  char*  seconds  = NULL;
  char*  timezone = NULL;
  int    dHours   = 0;
  int    dMinutes = 0;
  double dSeconds = 0;

  if (T != NULL)
  {
    *T   = 0;
    time = stringStrip(&T[1]);
  }

  char* tmp = strchr(date, '-');
  if (tmp == NULL)
    LM_RE(-1, ("delimiting hyphen between 'year' and 'month' not found: '%s'", timeString));
  *tmp = 0;

  month = &tmp[1];
  tmp = strchr(month, '-');

  if (tmp == NULL)
    LM_RE(-1, ("delimiting hyphen between 'month' and 'day' not found: '%s'", timeString));
  *tmp = 0;

  day = &tmp[1];

  if (T == NULL)
  {
    // If no 'T', then after the two chars of the day, the string must end
    if (day[2] != 0)
      LM_RE(-1, ("No 'T' present, but, the dateTime string doesn't end after YYYY-MM-DD: '%s'", timeString));
  }

  if (T != NULL)
  {
    hours = time;
    tmp   = strchr(time, ':');
    if (tmp == NULL)
      LM_RE(-1, ("delimiting colon between 'hours' and 'minutes' not found: '%s'", timeString));
    *tmp    = 0;
    minutes = &tmp[1];

    tmp = strchr(minutes, ':');
    if (tmp == NULL)
      LM_RE(-1, ("delimiting colon between 'minutes' and 'seconds' not found: '%s'", timeString));
    *tmp    = 0;
    seconds = &tmp[1];

    dSeconds = strtod(seconds, &timezone);
  }

  //
  // Now we have all the different parts of the timestamp, time to turn all of it into a unix timestamp, using timegm
  //
  struct tm tm;
  tm.tm_year   = atoi(year) - 1900;
  tm.tm_mon    = atoi(month) - 1;    // Month goes from 0 to 11
  tm.tm_mday   = atoi(day);
  tm.tm_hour   = (hours   != NULL)? atoi(hours)   : 0;
  tm.tm_min    = (minutes != NULL)? atoi(minutes) : 0;
  tm.tm_sec    = 0;   // Will be added later, as a floating point number

  int64_t secs      = timegm(&tm);
  double  timestamp = secs + dSeconds;

  //
  // Lastly, the timezone
  // Five possibilities:
  // * Nothing, not present
  // * Z (treated as nothing)
  // * +/- hh:mm
  // * +/- hhmm
  // * +/- hh
  //
  if ((timezone == NULL) || (*timezone == 0))
    return timestamp;

  if ((timezone[0] == 'Z') && (timezone[1] == 0))
    return timestamp;

  char sign = timezone[0];
  ++timezone;

  if ((sign != '-') && (sign != '+'))
    LM_RE(-1, ("Invalid timezone (none of { 'Z', '-', '+' }): '%s'", timeString));

  if ((strlen(timezone) == 5) && (timezone[2] == ':'))
  {
    timezone[2] = 0;
    dHours      = atoi(timezone);
    dMinutes    = atoi(&timezone[3]);
  }
  else if (strlen(timezone) == 4)
  {
    dMinutes    = atoi(&timezone[2]);
    timezone[2] = 0;
    dHours      = atoi(timezone);
  }
  else if (strlen(timezone) == 2)
    dHours = atoi(timezone);
  else
    LM_RE(-1, ("Invalid timezone: '%s'", timeString));

  if (sign == '+')
    timestamp = timestamp + dHours * 3600 + dMinutes * 60;
  else
    timestamp = timestamp - dHours * 3600 - dMinutes * 60;

  if (timestamp < 0)
    LM_RE(-1, ("timestamp before epoch is not allowed: '%s'", timeString));

  return timestamp;
}
