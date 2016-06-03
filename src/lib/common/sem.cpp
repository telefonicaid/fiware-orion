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
#include <map>  // for curl contexts

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/clockFunctions.h"



/* ****************************************************************************
*
* Globals -
*/
static sem_t           reqSem;
static sem_t           transSem;
static sem_t           cacheSem;
static sem_t           timeStatSem;
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



/* ****************************************************************************
*  curl context
*/


//
// Variables for mutexes and their state
//
// FIXME: contexts_mutex_errors and contexts_mutex2_errors are not yet used, see issue #2145
//
static std::map<std::string, struct curl_context>  contexts;
static pthread_mutex_t                             contexts_mutex         = PTHREAD_MUTEX_INITIALIZER;
static bool                                        contexts_mutex_taken   = false;
static int                                         contexts_mutex_errors  = 0;
static bool                                        contexts_mutex2_taken  = false;
static int                                         contexts_mutex2_errors = 0;


/* ****************************************************************************
*
* connectionContextSemGet - 
*/
const char* connectionContextSemGet(void)
{
  return (contexts_mutex_taken)? "taken" : "free";
}



/* ****************************************************************************
*
* connectionSubContextSemGet - 
*/
const char* connectionSubContextSemGet(void)
{
  return (contexts_mutex2_taken)? "taken" : "free";
}



// Statistics
static struct timespec accCCMutexTime = { 0, 0 };


/* ****************************************************************************
*
* curl_context_cleanup - 
*/
void curl_context_cleanup(void)
{
  for (std::map<std::string, struct curl_context>::iterator it = contexts.begin(); it != contexts.end(); ++it)
  {
    curl_easy_reset(it->second.curl);
    curl_easy_cleanup(it->second.curl);
    it->second.curl = NULL;
    release_curl_context(&it->second, true);
  }

  contexts.clear();
  curl_global_cleanup();
}



/* ****************************************************************************
*
* get_curl_context_reuse -
*/
static int get_curl_context_reuse(const std::string& key, struct curl_context* pcc)
{
  pcc->curl   = NULL;
  pcc->pmutex = NULL;

  int s = pthread_mutex_lock(&contexts_mutex);

  if (s != 0)
  {
    LM_E(("Runtime Error (pthread_mutex_lock failure)"));
    ++contexts_mutex_errors;
    return s;
  }
  contexts_mutex_taken = true;

  std::map<std::string, struct curl_context>::iterator it;
  it = contexts.find(key);
  if (it == contexts.end())
  {
    // not found, create it
    pcc->curl = curl_easy_init();
    if (pcc->curl != NULL)
    {
      pthread_mutex_t* pm = (pthread_mutex_t *) malloc(sizeof(*pm));

      if (pm == NULL)
      {
        pthread_mutex_unlock(&contexts_mutex);
        ++contexts_mutex_errors;
        contexts_mutex_taken = false;
        LM_E(("Runtime Error (malloc)"));
        return -1;
      }

      int s = pthread_mutex_init(pm, NULL);
      if (s != 0)
      {
        pthread_mutex_unlock(&contexts_mutex);
        contexts_mutex_taken = false;
        ++contexts_mutex_errors;
        LM_E(("Runtime Error (pthread_mutex_init)"));
        free(pm);
        return s;
      }
      pcc->pmutex   = pm;
      contexts[key] = *pcc;
    }
    else  // curl_easy returned null
    {
      pcc->pmutex = NULL;    // unnecessary but clearer
    }
  }
  else // previous context found
  {
    *pcc = it->second;
  }

  s = pthread_mutex_unlock(&contexts_mutex);
  if (s != 0)
  {
    ++contexts_mutex_errors;
    LM_E(("Runtime Error (pthread_mutex_unlock)"));
    return s;
  }

  contexts_mutex_taken = false;

  // lock the mutex, if everything was right
  // and cc is not {NULL, NULl}
  if (pcc->pmutex != NULL)
  {
    struct timespec  startTime;
    struct timespec  endTime;
    struct timespec  diffTime;

    if (semWaitStatistics)
    {
      clock_gettime(CLOCK_REALTIME, &startTime);
    }

    s = pthread_mutex_lock(pcc->pmutex);
    if (s != 0)
    {
      ++contexts_mutex2_errors;
      LM_E(("Runtime Error (pthread_mutex_lock)"));
      return s;
    }
    contexts_mutex2_taken = true;
    
    if (semWaitStatistics)
    {
      clock_gettime(CLOCK_REALTIME, &endTime);
      clock_difftime(&endTime, &startTime, &diffTime);

      int s = pthread_mutex_lock(&contexts_mutex);
      if (s != 0)
      {
        ++contexts_mutex_errors;
        LM_E(("Runtime Error (pthread_mutex_lock)"));
        return s;
      }
      contexts_mutex_taken = true;

      clock_addtime(&accCCMutexTime, &diffTime);
      s = pthread_mutex_unlock(&contexts_mutex);
      if (s != 0)
      {
        ++contexts_mutex_errors;
        LM_E(("Runtime Error (pthread_mutex_unlock)"));
        return s;
      }
      contexts_mutex_taken = false;
    }
  }

  return 0;
}

/* ****************************************************************************
*
* get_curl_context_new -
*/
static int get_curl_context_new(const std::string& key, struct curl_context* pcc)
{
  pcc->curl   = NULL;
  pcc->pmutex = NULL;

  pcc->curl = curl_easy_init();

  if (pcc->curl == NULL)
  {
    LM_E(("Runtime Error (curl_easy_init)"));
    return -1;
  }

  return 0;
}

/* ****************************************************************************
*
* get_curl_context -
*/
int get_curl_context(const std::string& key, struct curl_context* pcc)
{

  if (strcmp(notificationMode, "persistent") == 0)
  {
    LM_T(LmtCurlContext, ("using persistent curl_contexts"));
    return get_curl_context_reuse(key, pcc);
  }

  return get_curl_context_new(key, pcc);
}




/* ****************************************************************************
*
* release_curl_context_reuse -
*/
static int release_curl_context_reuse(struct curl_context *pcc, bool final)
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
    if (final)
    {
      free(pcc->pmutex);
    }

    if (s != 0)
    {
      LM_E(("Runtime Error (pthread_mutex_unlock)"));
      ++contexts_mutex2_errors;
      return s;
    }
    contexts_mutex2_taken = false;

    pcc->pmutex = NULL; // It will remain in global map
  }

  return 0;
}

/* ****************************************************************************
*
* release_curl_context_new -
*/
static int release_curl_context_new(struct curl_context *pcc, bool final)
{
  // Clean-up context if not an empty context
  if (pcc->curl != NULL)
  {
    curl_easy_cleanup(pcc->curl);
    pcc->curl = NULL;
  }

  return 0;
}

/* ****************************************************************************
*
* release_curl_context -
*/
int release_curl_context(struct curl_context *pcc, bool final)
{
  if (strcmp(notificationMode, "persistent") == 0)
  {
    return release_curl_context_reuse(pcc, final);
  }

  return release_curl_context_new(pcc, final);
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
* mutexTimeCCGet - get accumulated curl contexts mutex waiting time
*/
float mutexTimeCCGet(void)
{
  return accCCMutexTime.tv_sec + ((float) accCCMutexTime.tv_nsec) / 1E9;
}
