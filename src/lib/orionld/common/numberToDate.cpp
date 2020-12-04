/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <stdio.h>                                                  // sprintf
#include <string.h>                                                 // strlen
#include <time.h>                                                   // time, gmtime_r

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // Lmt*



// -----------------------------------------------------------------------------
//
// numberToDate -
//
bool numberToDate(double timestamp, char* date, int dateLen, char** detailsP)
{
  struct tm  tm;
  time_t     fromEpoch = (time_t) timestamp;
  double     millis    = timestamp - fromEpoch;

  gmtime_r(&fromEpoch, &tm);
  strftime(date, dateLen, "%Y-%m-%dT%H:%M:%S", &tm);

  int sLen = strlen(date);
  if (sLen + 5 >= dateLen)
  {
    LM_E(("Internal Error (not enough room for the decimals of the timestamp)"));
    return false;
  }

  int dMicros  = (int) (millis * 1000000) + 1;
  int dMillis  = dMicros / 1000;

  snprintf(&date[sLen], dateLen - sLen, ".%03dZ", dMillis);

  return true;
}
