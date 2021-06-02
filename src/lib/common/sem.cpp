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
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/clockFunctions.h"



/* ****************************************************************************
*
* Globals -
*/
static sem_t      reqSem;
static sem_t      transSem;
static sem_t      cacheSem;
static sem_t      timeStatSem;
static SemOpType  reqPolicy;



/* ****************************************************************************
*
* Time measuring variables - 
*/
static struct timespec accReqSemTime      = { 0, 0 };
static struct timespec accTransSemTime    = { 0, 0 };
static struct timespec accCacheSemTime    = { 0, 0 };
static struct timespec accTimeStatSemTime = { 0, 0 };



/* ****************************************************************************
*
* semInit -
*
*   parameter #2: 0 - the semaphore is to be shared between threads,
*   parameter #3: 1 - initially the semaphore is free
*
* RETURN VALUE (of sem_init)
*   0 on success,
*  -1 on failure
*
*/
int semInit(SemOpType _reqPolicy, bool semTimeStat, int shared, int takenInitially)
{
  if (sem_init(&reqSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'req' semaphore: %s)", strerror(errno)));
    return -1;
  }

  if (sem_init(&transSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'transactionId' semaphore: %s)", strerror(errno)));
    return -1;
  }

  if (sem_init(&cacheSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'cache' semaphore: %s)", strerror(errno)));
    return -1;
  }

  if (sem_init(&timeStatSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'timeStat' semaphore: %s)", strerror(errno)));
    return -1;
  }

  reqPolicy = _reqPolicy;

  // Measure accumulated semaphore waiting time?
  semWaitStatistics = semTimeStat;
  return 0;
}



/* ****************************************************************************
*
* reqSemTryToTake - try to take take semaphore
*/
int reqSemTryToTake(void)
{
  int r = sem_trywait(&reqSem);

  return r;
}



/* ****************************************************************************
*
* reqSemTake -
*/
int reqSemTake(const char* who, const char* what, SemOpType reqType, bool* taken)
{
  int r;

  if (reqPolicy == SemNoneOp)
  {
    *taken = false;
    return -1;
  }

  if ((reqPolicy == SemWriteOp) && (reqType == SemReadOp))
  {
    *taken = false;
    return -1;
  }

  if ((reqPolicy == SemReadOp) && (reqType == SemWriteOp))
  {
    *taken = false;
    return -1;
  }

  LM_T(LmtReqSem, ("%s taking the 'req' semaphore for '%s'", who, what));

  struct timespec startTime;
  struct timespec endTime;
  struct timespec diffTime;

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&reqSem);

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&accReqSemTime, &diffTime);
  }

  LM_T(LmtReqSem, ("%s has the 'req' semaphore", who));

  *taken = true;
  return r;
}



/* ****************************************************************************
*
* semTimeReqGet - get accumulated req semaphore waiting time
*/
float semTimeReqGet(void)
{
  return accReqSemTime.tv_sec + ((float) accReqSemTime.tv_nsec) / 1E9;
}



/* ****************************************************************************
*
* semTimeTransGet - get accumulated trans semaphore waiting time
*/
float semTimeTransGet(void)
{
  return accTransSemTime.tv_sec + ((float) accTransSemTime.tv_nsec) / 1E9;
}



/* ****************************************************************************
*
* semTimeCacheGet - get accumulated cache semaphore waiting time
*/
float semTimeCacheGet(void)
{
  return accCacheSemTime.tv_sec + ((float) accCacheSemTime.tv_nsec)/ 1E9;
}



/* ****************************************************************************
*
* semTimeTimeStatGet - get accumulated trans semaphore waiting time
*/
float semTimeTimeStatGet(void)
{
  return accTimeStatSemTime.tv_sec + ((float) accTimeStatSemTime.tv_nsec) / 1E9;
}



/* ****************************************************************************
*
* semTimeReqReset - 
*/
void semTimeReqReset(void)
{
  accReqSemTime.tv_sec  = 0;
  accReqSemTime.tv_nsec = 0;
}



/* ****************************************************************************
*
* semTimeTransReset - 
*/
void semTimeTransReset(void)
{
  accTransSemTime.tv_sec  = 0;
  accTransSemTime.tv_nsec = 0;
}



/* ****************************************************************************
*
* semTimeCacheReset - 
*/
void semTimeCacheReset(void)
{
  accCacheSemTime.tv_sec  = 0;
  accCacheSemTime.tv_nsec = 0;
}



/* ****************************************************************************
*
* semTimeTimeStatReset - 
*/
void semTimeTimeStatReset(void)
{
  accTimeStatSemTime.tv_sec  = 0;
  accTimeStatSemTime.tv_nsec = 0;
}



/* ****************************************************************************
*
* transSemTake -
*/
int transSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtTransSem, ("%s taking the 'trans' semaphore for '%s'", who, what));

  struct timespec startTime;
  struct timespec endTime;
  struct timespec diffTime;

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&transSem);

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&accTransSemTime, &diffTime);
  }

  LM_T(LmtTransSem, ("%s has the 'trans' semaphore", who));

  return r;
}



/* ****************************************************************************
*
* cacheSemTake -
*/
int cacheSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtCacheSem, ("%s taking the 'cache' semaphore for '%s'", who, what));

  struct timespec startTime;
  struct timespec endTime;
  struct timespec diffTime;

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&cacheSem);

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&accCacheSemTime, &diffTime);
  }

  LM_T(LmtCacheSem, ("%s has the 'cache' semaphore", who));

  return r;
}



/* ****************************************************************************
*
* reqSemGive -
*/
int reqSemGive(const char* who, const char* what, bool semTaken)
{
  if (semTaken == false)
  {
    return 0;
  }

  if (what != NULL)
  {
    LM_T(LmtReqSem, ("%s gives the 'req' semaphore for '%s'", who, what));
  }
  else
  {
    LM_T(LmtReqSem, ("%s gives the 'req' semaphore", who));
  }

  return sem_post(&reqSem);
}



/* ****************************************************************************
*
* transSemGive -
*/
int transSemGive(const char* who, const char* what)
{
  if (what != NULL)
  {
    LM_T(LmtTransSem, ("%s gives the 'trans' semaphore for '%s'", who, what));
  }
  else
  {
    LM_T(LmtTransSem, ("%s gives the 'trans' semaphore", who));
  }

  return sem_post(&transSem);
}



/* ****************************************************************************
*
* cacheSemGive -
*/
int cacheSemGive(const char* who, const char* what)
{
  if (what != NULL)
  {
    LM_T(LmtCacheSem, ("%s gives the 'cache' semaphore for '%s'", who, what));
  }
  else
  {
    LM_T(LmtCacheSem, ("%s gives the 'cache' semaphore", who));
  }

  return sem_post(&cacheSem);
}



/* ****************************************************************************
*
* timeStatSemTake -
*/
int timeStatSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtTimeStatSem, ("%s taking the 'timeStat' semaphore for '%s'", who, what));

  struct timespec startTime;
  struct timespec endTime;
  struct timespec diffTime;

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&timeStatSem);

  if (semWaitStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&accTimeStatSemTime, &diffTime);
  }

  LM_T(LmtTimeStatSem, ("%s has the 'timeStat' semaphore", who));

  return r;
}



/* ****************************************************************************
*
* timeStatSemGive -
*/
int timeStatSemGive(const char* who, const char* what)
{
  if (what != NULL)
  {
    LM_T(LmtTimeStatSem, ("%s gives the 'timeStat' semaphore for '%s'", who, what));
  }
  else
  {
    LM_T(LmtTimeStatSem, ("%s gives the 'timeStat' semaphore", who));
  }

  return sem_post(&timeStatSem);
}



/* ****************************************************************************
*
* reqSemGet - 
*/
const char* reqSemGet(void)
{
  int value;

  if (sem_getvalue(&reqSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }
  
  return "free";  
}



/* ****************************************************************************
*
* transSemGet - 
*/
const char* transSemGet(void)
{
  int value;

  if (sem_getvalue(&transSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }
  
  return "free";  
}



/* ****************************************************************************
*
* cacheSemGet - 
*/
const char* cacheSemGet(void)
{
  int value;

  if (sem_getvalue(&cacheSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }
  
  return "free";  
}



/* ****************************************************************************
*
* timeStatSemGet - 
*/
const char* timeStatSemGet(void)
{
  int value;

  if (sem_getvalue(&timeStatSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }
  
  return "free";  
}



