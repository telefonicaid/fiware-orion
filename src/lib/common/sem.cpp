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

// includes for curl contexts
#include <map>



/* ****************************************************************************
*
* Globals -
*/
static sem_t           reqSem;
static sem_t           transSem;
static SemRequestType  reqPolicy;



/* ****************************************************************************
*
* Time measuring variables - 
*/
static struct timespec accReqSemTime   = { 0, 0 };
static struct timespec accTransSemTime = { 0, 0 };



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
int semInit(SemRequestType _reqPolicy, bool semTimeStat, int shared, int takenInitially)
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

  reqPolicy = _reqPolicy;

  // Measure accumulated semaphore waiting time?
  semTimeStatistics = semTimeStat;
  return 0;
}



/* ****************************************************************************
*
* reqSemTake -
*/
int reqSemTake(const char* who, const char* what, SemRequestType reqType, bool* taken)
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

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&reqSem);

  if (semTimeStatistics)
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
void semTimeReqGet(char* buf, int bufLen)
{
  if (semTimeStatistics)
  {
    snprintf(buf, bufLen, "%lu.%09d", accReqSemTime.tv_sec, (int) accReqSemTime.tv_nsec);
  }
  else
  {
    snprintf(buf, bufLen, "Disabled");
  }
}



/* ****************************************************************************
*
* semTimeTransGet - get accumulated trans semaphore waiting time
*/
void semTimeTransGet(char* buf, int bufLen)
{
  if (semTimeStatistics)
  {
    snprintf(buf, bufLen, "%lu.%09d", accTransSemTime.tv_sec, (int) accTransSemTime.tv_nsec);
  }
  else
  {
    snprintf(buf, bufLen, "Disabled");
  }
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
* transSemTake -
*/
int transSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtTransSem, ("%s taking the 'trans' semaphore for '%s'", who, what));

  struct timespec startTime;
  struct timespec endTime;
  struct timespec diffTime;

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  r = sem_wait(&transSem);

  if (semTimeStatistics)
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
 *  curl context
 */


static pthread_mutex_t contexts_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::map<std::string, struct curl_context> contexts;

//Statistics
static struct timespec accCCMutexTime = { 0, 0 };


int get_curl_context(const std::string& key, struct curl_context *pcc)
{
  pcc->curl = NULL;
  pcc->pmutex = NULL;
  int s = pthread_mutex_lock(&contexts_mutex);
  if(s!=0)
  {
       LM_E(("pthread_mutex_lock"));
       return s;
  }
  std::map<std::string, struct curl_context>::iterator it;
  it = contexts.find(key);
  if (it==contexts.end())
  {
      //not found, create it
      pcc->curl = curl_easy_init();
      if (pcc->curl != NULL)
      {
        pthread_mutex_t *pm = (pthread_mutex_t *) malloc(sizeof(*pm));
        if (pm==NULL)
        {
          LM_E(("malloc"));
          return -1;
        }
        int s = pthread_mutex_init(pm, NULL);
        if(s!=0)
        {
           LM_E(("pthread_mutex_init"));
           free(pm);
           return s;
        }
        pcc->pmutex = pm;
        contexts[key] = *pcc;
      }
      else  // curl_easy returned null
      {
       pcc->pmutex = NULL;    //unnecessary but clearer
      }
  }
  else //previuos context found
  {
      *pcc=it->second;
  }
  s = pthread_mutex_unlock(&contexts_mutex);
  if(s!=0)
  {
    LM_E(("pthread_mutex_unlock"));
    return s;
  }

  // lock the mutex, if everything was right
  // and cc is not {NULL, NULl}
  if (pcc->pmutex != NULL) {

      struct timespec startTime;
      struct timespec endTime;
      struct timespec diffTime;

      if (semTimeStatistics)
      {
        clock_gettime(CLOCK_REALTIME, &startTime);
      }

      s = pthread_mutex_lock(pcc->pmutex);
      if(s!=0)
      {
        LM_E(("pthread_mutex_lock"));
        return s;
      }

      if (semTimeStatistics)
      {
        clock_gettime(CLOCK_REALTIME, &endTime);
        clock_difftime(&endTime, &startTime, &diffTime);
        int s = pthread_mutex_lock(&contexts_mutex);
        if(s!=0)
        {
             LM_E(("pthread_mutex_lock"));
             return s;
        }
        clock_addtime(&accCCMutexTime, &diffTime);
        s = pthread_mutex_unlock(&contexts_mutex);
        if(s!=0)
        {
          LM_E(("pthread_mutex_unlock"));
          return s;
        }

      }
  }
  return 0;
}


int release_curl_context(struct curl_context *pcc)
{
  // Reset context if not an empty context
  if (pcc->curl != NULL)
  {
    curl_easy_reset(pcc->curl);
    pcc->curl = NULL; // It will remain in global map
  }
  // Unlock the mutex if not an empty context
  if (pcc->pmutex != NULL)
    {
    int s = pthread_mutex_unlock(pcc->pmutex);
    if(s!=0)
    {
      LM_E(("pthread_mutex_unlock"));
      return s;
    }
    pcc->pmutex = NULL; // It will remain in global map
  }

  return 0;
}


/* ****************************************************************************
*
* mutexTimeCCReset -
*/
void mutexTimeCCReset(void)
{
  accCCMutexTime.tv_sec  = 0;
  accCCMutexTime.tv_nsec = 0;
}

/* ****************************************************************************
*
* mutexTimeCCGet - get accumulated curl contexts mutext waiting time
*/
void mutexTimeCCGet(char* buf, int bufLen)
{
  if (semTimeStatistics)
  {
    snprintf(buf, bufLen, "%lu.%09d", accCCMutexTime.tv_sec, (int) accCCMutexTime.tv_nsec);
  }
  else
  {
    snprintf(buf, bufLen, "Disabled");
  }
}

