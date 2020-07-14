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
* Author: Fermin Galan
*/
#include <stdint.h>
#include <errno.h>                   // errno
#include <string.h>                  // strerror

#include "common/Timer.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"



/* ****************************************************************************
*
* Timer::~Timer -
*/
Timer::~Timer(void)
{
  //
  // FIXME: This destructor is needed to avoid warning message.
  // Compilation fails when a warning occurs, and it is enabled
  // compilation option -Werror "warnings being treated as errors"
  //
  LM_T(LmtNotImplemented, ("Timer destructor not implemented"));
}



/* ****************************************************************************
*
* Timer::getCurrentTime -
*/
double Timer::getCurrentTime(void)
{
  struct timespec  ts;
  double           timestamp;

  if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
  {
    LM_E(("Internal Error (clock_gettime: %s)", strerror(errno)));
    return 0;
  }

  timestamp = ts.tv_sec + ((double) ts.tv_nsec) / 1000000000;

  return timestamp;
}
