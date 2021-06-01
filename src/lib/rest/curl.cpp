/*
*
* Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/clockFunctions.h"
#include "curl.h"



/* ****************************************************************************
*  curl context
*/
//
// Variables for mutexes and their state
//
// FIXME: contexts_mutex_errors and endpoint_mutexes_errors are not yet used, see issue #2145
//
static std::map<std::string, struct curl_context>  contexts;
static pthread_mutex_t                             contexts_mutex          = PTHREAD_MUTEX_INITIALIZER;
static bool                                        contexts_mutex_taken    = false;
static int                                         contexts_mutex_errors   = 0;
static int                                         endpoint_mutexes_taken  = 0;
static int                                         endpoint_mutexes_errors = 0;



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

  endpoint_mutexes_taken = 0;
  contexts_mutex_taken   = false;
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
      ++endpoint_mutexes_errors;
      LM_E(("Runtime Error (pthread_mutex_lock)"));
      return s;
    }
    ++endpoint_mutexes_taken;
    
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

      //clock_addtime(&accCCMutexTime, &diffTime);
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
      ++endpoint_mutexes_errors;
      return s;
    }
    --endpoint_mutexes_taken;

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

static struct timespec accCCMutexTime = { 0, 0 };



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
