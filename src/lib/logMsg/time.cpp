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
* Author: developer
*/
#include <stdint.h>             // int64, ...

#include "logMsg/time.h"        // Own interface



namespace lm
{
  // Extracted from udftime.c implementation

#ifndef __isleap

//
// Nonzero if YEAR is a leap year (every 4 years,
// except every 100th isn't, and every 400th is).
//
#define __isleap(year)  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

#endif


/* How many days come before each month (0-12)? */
const uint16_t __mon_yday[2][13] =
{
  /* Normal years.  */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },

  /* Leap years.  */
  { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define MAX_YEAR_SECONDS     69
#define SPD                  0x15180 /* 3600 * 24 */
#define SPY(y, l, s)         (SPD * (365 * y + l) + s)


time_t year_seconds[MAX_YEAR_SECONDS] =
{
  /*1970*/ SPY(0,   0, 0), SPY(1,   0, 0), SPY(2,   0, 0), SPY(3,   1, 0),
  /*1974*/ SPY(4,   1, 0), SPY(5,   1, 0), SPY(6,   1, 0), SPY(7,   2, 0),
  /*1978*/ SPY(8,   2, 0), SPY(9,   2, 0), SPY(10,  2, 0), SPY(11,  3, 0),
  /*1982*/ SPY(12,  3, 0), SPY(13,  3, 0), SPY(14,  3, 0), SPY(15,  4, 0),
  /*1986*/ SPY(16,  4, 0), SPY(17,  4, 0), SPY(18,  4, 0), SPY(19,  5, 0),
  /*1990*/ SPY(20,  5, 0), SPY(21,  5, 0), SPY(22,  5, 0), SPY(23,  6, 0),
  /*1994*/ SPY(24,  6, 0), SPY(25,  6, 0), SPY(26,  6, 0), SPY(27,  7, 0),
  /*1998*/ SPY(28,  7, 0), SPY(29,  7, 0), SPY(30,  7, 0), SPY(31,  8, 0),
  /*2002*/ SPY(32,  8, 0), SPY(33,  8, 0), SPY(34,  8, 0), SPY(35,  9, 0),
  /*2006*/ SPY(36,  9, 0), SPY(37,  9, 0), SPY(38,  9, 0), SPY(39, 10, 0),
  /*2010*/ SPY(40, 10, 0), SPY(41, 10, 0), SPY(42, 10, 0), SPY(43, 11, 0),
  /*2014*/ SPY(44, 11, 0), SPY(45, 11, 0), SPY(46, 11, 0), SPY(47, 12, 0),
  /*2018*/ SPY(48, 12, 0), SPY(49, 12, 0), SPY(50, 12, 0), SPY(51, 13, 0),
  /*2022*/ SPY(52, 13, 0), SPY(53, 13, 0), SPY(54, 13, 0), SPY(55, 14, 0),
  /*2026*/ SPY(56, 14, 0), SPY(57, 14, 0), SPY(58, 14, 0), SPY(59, 15, 0),
  /*2030*/ SPY(60, 15, 0), SPY(61, 15, 0), SPY(62, 15, 0), SPY(63, 16, 0),
  /*2034*/ SPY(64, 16, 0), SPY(65, 16, 0), SPY(66, 16, 0), SPY(67, 17, 0),
  /*2038*/ SPY(68, 17, 0)
};


#define SECS_PER_HOUR (60 * 60)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24)



/* ****************************************************************************
*
* gmtime_r -
*/
struct tm* gmtime_r(time_t* t, tm* tp)
{
  // extracted from __offtime implementation
  int64_t          offset = 0;
  int64_t          days;
  int64_t          rem;
  int64_t          y;
  const uint16_t*  ip;

  days = *t / SECS_PER_DAY;
  rem  = *t % SECS_PER_DAY;
  rem += offset;

  while (rem < 0)
  {
    rem += SECS_PER_DAY;
    --days;
  }

  while (rem >= SECS_PER_DAY)
  {
    rem -= SECS_PER_DAY;
    ++days;
  }

  tp->tm_hour = rem / SECS_PER_HOUR;
  rem %= SECS_PER_HOUR;
  tp->tm_min = rem / 60;
  tp->tm_sec = rem % 60;

  /* January 1, 1970 was a Thursday.  */
  tp->tm_wday = (4 + days) % 7;
  if (tp->tm_wday < 0)
  {
    tp->tm_wday += 7;
  }

  y = 1970;

#define DIV(a, b)            ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

  while (days < 0 || days >= (__isleap (y) ? 366 : 365))
  {
    /* Guess a corrected year, assuming 365 days per year.  */
    int64_t  yg = y + days / 365 - (days % 365 < 0);

    /* Adjust DAYS and Y to match the guessed year.  */
    days -= ((yg - y) * 365
             + LEAPS_THRU_END_OF(yg - 1)
             - LEAPS_THRU_END_OF(y - 1));
    y = yg;
  }

  tp->tm_year = y - 1900;
  if (tp->tm_year != y - 1900)
  {
    // The year cannot be represented due to overflow.
    // __set_errno (EOVERFLOW);
    return NULL;
  }

  tp->tm_yday = days;
  ip = __mon_yday[__isleap(y)];
  for (y = 11; days < (int64_t) ip[y]; --y)
  {
    continue;
  }

  days -= ip[y];
  tp->tm_mon = y;
  tp->tm_mday = days + 1;
  return tp;
}
}
