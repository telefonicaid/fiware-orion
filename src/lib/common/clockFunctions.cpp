/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "clockFunctions.h"



/* ****************************************************************************
*
* clock_difftime - 
*/
void clock_difftime(struct timespec* endTime, struct timespec* startTime, struct timespec* diffTime)
{
  diffTime->tv_nsec = endTime->tv_nsec - startTime->tv_nsec;
  diffTime->tv_sec  = endTime->tv_sec  - startTime->tv_sec;

  if (diffTime->tv_nsec < 0)
  {
    diffTime->tv_sec -= 1;
    diffTime->tv_nsec += 1000000000;
  }
}



/* ****************************************************************************
*
* clock_addtime - 
*/
void clock_addtime(struct timespec* accTime, struct timespec* diffTime)
{
  accTime->tv_nsec += diffTime->tv_nsec;
  accTime->tv_sec  += diffTime->tv_sec;

  if (accTime->tv_nsec >= 1000000000)
  {
    accTime->tv_sec  += 1;
    accTime->tv_nsec -= 1000000000;
  }
}



/* ****************************************************************************
*
* clock_subtime - 
*/
void clock_subtime(struct timespec* subtrahend, const struct timespec* minuend)
{
  subtrahend->tv_sec  -= minuend->tv_sec;
  subtrahend->tv_nsec -= minuend->tv_nsec;

  if (subtrahend->tv_nsec < 0)
  {
    subtrahend->tv_sec  -= 1;
    subtrahend->tv_nsec += 1000000000;
  }
}
