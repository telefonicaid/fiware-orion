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
// dateParse -
//
// YYYY-MM-DD
// YYYYMMDD
// YYYY-MM
//
static bool dateParse(const char* dateTime, char* dateString, int* yearP, int* monthP, int* dayP, char* errorString, int errorStringLen)
{
  int len     = strlen(dateString);
  int year    = 0;
  int month   = 0;
  int day     = 0;

  LM_T(LmtDateTime, ("len('%s'): %d", dateString, len));

  if (len == 10)
  {
    const char* format = "dddd-dd-dd";

    for (int ix = 0; ix < 10; ix++)
    {
      if ((format[ix] == 'd') && ((dateString[ix] < '0') || (dateString[ix] > '9')))
      {
        snprintf(errorString, errorStringLen, "error in position %d of DATE part '%s' of DateTime '%s': not a digit", ix, dateString, dateTime);
        return false;
      }
      else if ((format[ix] == '-') && (dateString[ix] != '-'))
      {
        snprintf(errorString, errorStringLen, "error in position %d of DATE part '%s' of DateTime '%s': expected a hyphen", ix, dateString, dateTime);
        return false;
      }
    }

    dateString[4] = 0;
    dateString[7] = 0;

    year  = atoi(dateString);
    month = atoi(&dateString[5]);
    day   = atoi(&dateString[8]);

    LM_T(LmtDateTime, ("year:  %d", year));
    LM_T(LmtDateTime, ("month: %d", month));
    LM_T(LmtDateTime, ("day:   %d", day));
  }
  else if (len == 8)
  {
    for (int ix = 0; ix < 8; ix++)
    {
      if ((dateString[ix] < '0') || (dateString[ix] > '9'))
      {
        snprintf(errorString, errorStringLen, "error in position %d of DATE part '%s' of DateTime '%s': not a digit", ix, dateString, dateTime);
        return false;
      }
    }
  }
  else if (len == 7)
  {
    const char* format = "dddd-dd";

    for (int ix = 0; ix < 7; ix++)
    {
      if ((format[ix] == 'd') && ((dateString[ix] < '0') || (dateString[ix] > '9')))
      {
        snprintf(errorString, errorStringLen, "error in position %d of DATE part '%s' of DateTime '%s': not a digit", ix, dateString, dateTime);
        return false;
      }
      else if ((format[ix] == '-') && (dateString[ix] != '-'))
      {
        snprintf(errorString, errorStringLen, "error in position %d of DATE part '%s' of DateTime '%s': expected a hyphen", ix, dateString, dateTime);
        return false;
      }
    }

    month = atoi(&dateString[4]);
    dateString[4] = 0;
    year  = atoi(dateString);
  }
  else
  {
    snprintf(errorString, errorStringLen, "invalid DateTime: '%s' (error in 'date' part '%s')", dateTime, dateString);
    return false;
  }

  if (year < 1970)
  {
    snprintf(errorString, errorStringLen, "error in 'year' part of DateTime '%s': invalid year '%d' (pre 1970)", dateTime, year);
    return false;
  }

  if ((month <= 0) || (month > 12))
  {
    snprintf(errorString, errorStringLen, "error in 'month' part of DateTime '%s': invalid month '%d' (range: 01..12)", dateTime, month);
    return false;
  }
  --month;   // Month goes from 0 to 11

  if ((day <= 0) || (day > 31))
  {
    snprintf(errorString, errorStringLen, "error in 'day' part of DateTime '%s': invalid day '%d' (range: 01..31)", dateTime, day);
    return false;
  }

  const char* months[12]  = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
  const int   maxDays[12] = {      31,       29,       31,      30,     31,     30,     31,      31,        30,         31,         30,         31     };

  if (day > maxDays[month])
  {
    snprintf(errorString, errorStringLen, "error in 'day' part of DateTime '%s': invalid day '%d' (max %d days in %s)", dateTime, day, maxDays[month], months[month]);
    return false;
  }

  *yearP  = year;
  *monthP = month;
  *dayP   = day;

  return true;
}



// -----------------------------------------------------------------------------
//
// timeParse -
//
// HH
// HH:mm
// HH:mm:SS
// HH:mm:SS.sss
// HHmm
// HHmmSS
// HHmmSS.sss
//
static bool timeParse(const char* dateTime, char* timeString, int* hourP, int* minuteP, double* secondP, char* errorString, int errorStringLen)
{
  if (timeString[0] == 0)
    return true;

  int          len      = strlen(timeString);
  int          hour     = 0;
  int          minute   = 0;
  double       second   = 0;
  bool         extended = timeString[2] == ':';
  const char*  format = (extended == true)? "dd:dd:dd.dddddd" : "dddddd.dddddd";

  for (int ix = 0; ix < len; ix++)
  {
    if ((format[ix] == 'd') && (timeString[ix] < '0' || timeString[ix] > '9'))
    {
      snprintf(errorString, errorStringLen, "error in position %d of TIME part '%s' of DateTime '%s': not a digit", ix, timeString, dateTime);
      return false;
    }
    else if ((format[ix] == ':') && (timeString[ix] != ':'))
    {
      snprintf(errorString, errorStringLen, "error in position %d of TIME part '%s' of DateTime '%s': expected a colon", ix, timeString, dateTime);
      return false;
    }
    else if ((format[ix] == '.') && (timeString[ix] != '.'))
    {
      snprintf(errorString, errorStringLen, "error in position %d of TIME part '%s' of DateTime '%s': expected a dot", ix, timeString, dateTime);
      return false;
    }
    else if (format[ix] == 0)
    {
      snprintf(errorString, errorStringLen, "error in position %d of TIME part '%s' of DateTime '%s': expected end-of-string", ix, timeString, dateTime);
      return false;
    }
  }

  if (len ==  2)
    hour = atoi(timeString);
  else if (extended == true)
  {
    if (len == 5)  // HH:mm
    {
      timeString[2] = 0;
      hour   = atoi(timeString);
      minute = atoi(&timeString[3]);
    }
    else if (len == 8)  // HH:mm:SS
    {
      timeString[2] = 0;
      timeString[5] = 0;
      hour   = atoi(timeString);
      minute = atoi(&timeString[3]);
      second = atoi(&timeString[6]);
    }
    else if (len >= 10)  // HH:mm:SS.ssssss
    {
      timeString[2] = 0;
      timeString[5] = 0;
      hour   = atoi(timeString);
      minute = atoi(&timeString[3]);
      second = strtod(&timeString[6], NULL);
    }
    else
    {
      snprintf(errorString, errorStringLen, "invalid DateTime: '%s' (error in 'date' part)", dateTime);
      return false;
    }
  }
  else
  {
    if (len == 4)  // HHmm
    {
      minute = atoi(&timeString[2]);
      timeString[2] = 0;
      hour   = atoi(timeString);
    }
    else if (len == 6)  // HHmmSS
    {
      second = atoi(&timeString[4]);
      timeString[4] = 0;
      minute = atoi(&timeString[2]);
      timeString[2] = 0;
      hour   = atoi(timeString);
    }
    else if (len > 7)  // HHmmSS.ssssss
    {
      second = strtod(&timeString[4], NULL);
      timeString[4] = 0;
      minute = atoi(&timeString[2]);
      timeString[2] = 0;
      hour   = atoi(timeString);
    }
    else
    {
      snprintf(errorString, errorStringLen, "invalid DateTime: '%s' (error in 'date' part)", dateTime);
      return false;
    }
  }

  if (hour > 23)
  {
    snprintf(errorString, errorStringLen, "error in 'hour' part of DateTime '%s': invalid hour '%d' (range: 00..22)", dateTime, hour);
    return false;
  }

  if (minute > 59)
  {
    snprintf(errorString, errorStringLen, "error in 'minute' part of DateTime '%s': invalid minute '%d' (range: 00..59)", dateTime, minute);
    return false;
  }

  if (second >= 60)
  {
    snprintf(errorString, errorStringLen, "error in 'second' part of DateTime '%s': invalid second '%.3f' (range: 00..59.999)", dateTime, second);
    return false;
  }

  *hourP   = hour;
  *minuteP = minute;
  *secondP = second;

  return true;
}



// -----------------------------------------------------------------------------
//
// timezoneParse -
//
// Z
// +HH
// -HH
// +HH:mm
// +HHmm
// -HH:mm
// -HHmm
//
static bool timezoneParse(const char* dateTime, char* timezoneString, int* hourP, int* minuteP, char* signP, char* errorString, int errorStringLen)
{
  if (timezoneString[0] == 0)
    return true;

  *hourP   = 0;
  *minuteP = 0;
  *signP   = 'Z';

  if (timezoneString[0] == 'Z')
  {
    if (timezoneString[1] == 0)
      return true;

    snprintf(errorString, errorStringLen, "trailing garbage after 'Z' in TIMEZONE part '%s' of DateTime '%s'", timezoneString, dateTime);
    return false;
  }

  if      (timezoneString[0] == '-') *signP = '-';
  else if (timezoneString[0] == '+') *signP = '+';
  else
  {
    snprintf(errorString, errorStringLen, "unsupported character in position 0 of TIMEZONE part '%s' of DateTime '%s'", timezoneString, dateTime);
    return false;
  }

  ++timezoneString;

  int len = strlen(timezoneString);

  if ((len == 2) || (len == 4))
  {
    for (int ix = 0; ix < len; ++ix)
    {
      if ((timezoneString[ix] < '0' || timezoneString[ix] > '9'))
      {
        snprintf(errorString, errorStringLen, "error in position %d of TIMEZONE part '%s' of DateTime '%s': not a digit", ix, timezoneString, dateTime);
        return false;
      }
    }

    if (len == 4)
    {
      *minuteP = atoi(&timezoneString[2]);
      timezoneString[2] = 0;
    }

    *hourP = atoi(timezoneString);
  }
  else if (len == 5)
  {
    for (int ix = 0; ix < len; ++ix)
    {
      if ((ix != 2) && ((timezoneString[ix] < '0' || timezoneString[ix] > '9')))
      {
        snprintf(errorString, errorStringLen, "error in position %d of TIMEZONE part '%s' of DateTime '%s': not a digit", ix, timezoneString, dateTime);
        return false;
      }
      else if ((ix == 2) && (timezoneString[ix] != ':'))
      {
        snprintf(errorString, errorStringLen, "error in position %d of TIMEZONE part '%s' of DateTime '%s': expected a colon", ix, timezoneString, dateTime);
        return false;
      }
    }

    timezoneString[2] = 0;
    *hourP   = atoi(timezoneString);
    *minuteP = atoi(&timezoneString[3]);
  }
  else
  {
    snprintf(errorString, errorStringLen, "invalid TIMEZONE part '%s' of DateTime '%s')", timezoneString, dateTime);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// dateTimeFromString -
//
// Supported formats:
// - YYYY-MM-DD
// - YYYY-MM-DD T HH
// - YYYY-MM-DD T HH Z
// - YYYY-MM-DD T HH:mm
// - YYYY-MM-DD T HH:mm Z
// - YYYY-MM-DD T HHmm
// - YYYY-MM-DD T HHmm Z
// - YYYY-MM-DD T HH:mm:SS
// - YYYY-MM-DD T HH:mm:SS Z
// - YYYY-MM-DD T HHmmSS
// - YYYY-MM-DD T HHmmSS Z
// - YYYY-MM-DD T HH:mm:SS.mmm
// - YYYY-MM-DD T HH:mm:SS.mmmZ
// - YYYY-MM-DD T HHmmSS.mmm
// - YYYY-MM-DD T HHmmSS.mmmZ
//
// Instead of "Z" we can have:
// - +HH
// - -HH
// - +HH:mm
// - +HHmm
// - -HH:mm
// - -HHmm
//
// To be able to parse all this, the following three parts must be extracted:
// - DATE
// - TIME
// - TIMEZONE
//
double dateTimeFromString(const char* iso8601String, char* errorString, int errorStringLen)
{
  char iso8601[64];

  strncpy(iso8601, iso8601String, sizeof(iso8601) - 1);

  char* date = stringStrip(iso8601);
  char* T    = strchr(date, 'T');

  LM_T(LmtDateTime, ("In: '%s'", iso8601));

  //
  // Extracting dateString. timeString, and timezoneString
  //
  char dateString[11];
  char timeString[30];
  char timezoneString[7];

  bzero(dateString,     sizeof(dateString));
  bzero(timeString,     sizeof(timeString));
  bzero(timezoneString, sizeof(timezoneString));

  if (T == NULL)
  {
    int    year      = 0;
    int    month     = 0;
    int    day       = 0;

    //
    // Is it a correct date and nothing more?
    // Need to use a copy of the datetime string as dateParse is destructive
    //
    char iso8601_2[64];
    strncpy(iso8601_2, date, sizeof(iso8601_2) - 1);

    if (dateParse(iso8601String, iso8601_2, &year, &month, &day, errorString, errorStringLen) == false)
      LM_RE(-1, ("Error parsing ISO8601 timestamp '%s': %s", iso8601String, errorString));


    strncpy(dateString, date, sizeof(dateString) - 1);
  }
  else
  {
    *T = 0;
    ++T;
    strncpy(dateString, date, sizeof(dateString) - 1);

    // Find 'Z', '+', or '-'
    char* tzStart = strchr(T, 'Z');
    if (tzStart == NULL) tzStart = strchr(T, '+');
    if (tzStart == NULL) tzStart = strchr(T, '-');

    if (tzStart == NULL)
      strncpy(timeString, T, sizeof(timeString));
    else
    {
      strncpy(timezoneString, tzStart, sizeof(timezoneString));
      *tzStart = 0;
      strncpy(timeString, T, sizeof(timeString));
    }
  }

  LM_T(LmtDateTime, ("dateString;     '%s'", dateString));
  LM_T(LmtDateTime, ("timeString;     '%s'", timeString));
  LM_T(LmtDateTime, ("timezoneString; '%s'", timezoneString));

  int    year      = 0;
  int    month     = 0;
  int    day       = 0;
  int    hour      = 0;
  int    minute    = 0;
  double secs      = 0;
  int    tzHour    = 0;
  int    tzMinute  = 0;
  char   sign      = 'Z';

  if (dateParse(iso8601String, dateString, &year, &month, &day, errorString, errorStringLen) == false)
    LM_RE(-1, ("Error parsing ISO8601 timestamp '%s': %s", iso8601String, errorString));

  if (timeParse(iso8601String, timeString, &hour, &minute, &secs, errorString, errorStringLen) == false)
    LM_RE(-1, ("Error parsing ISO8601 timestamp '%s': %s", iso8601String, errorString));

  if (timezoneParse(iso8601String, timezoneString, &tzHour, &tzMinute, &sign, errorString, errorStringLen) == false)
    LM_RE(-1, ("Error parsing ISO8601 timestamp '%s': %s", iso8601String, errorString));


  //
  // Now we have all the different parts of the timestamp, time to turn all of it into a unix timestamp, using timegm
  //
  struct tm tm;
  tm.tm_year   = year - 1900;
  tm.tm_mon    = month;
  tm.tm_mday   = day;
  tm.tm_hour   = hour;
  tm.tm_min    = minute;
  tm.tm_sec    = 0;   // Will be added later, as a floating point number

  int64_t dSecs      = timegm(&tm);
  double  timestamp  = dSecs + secs;
  int     tzSecs     = (tzHour * 3600 + tzMinute * 60) * ((sign == '-')? 1 : -1);

  timestamp += tzSecs;
  return timestamp;
}
