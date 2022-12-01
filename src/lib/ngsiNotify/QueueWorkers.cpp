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
* Author: Orion dev team
*/
#include <pthread.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsiNotify/QueueWorkers.h"

#include "ngsiNotify/doNotify.h"



/* ****************************************************************************
*
* workerFunc - prototype
*/
static void* workerFunc(void* pSyncQ);



/* ****************************************************************************
*
* QueueWorkers::start() -
*/
int QueueWorkers::start()
{
  for (int i = 0; i < numberOfThreads; ++i)
  {
    pthread_t  tid;
    int        rc = pthread_create(&tid, NULL, workerFunc, pQueue);

    if (rc != 0)
    {
      LM_E(("Runtime Error (pthread_create: %s)", strerror(errno)));
      return rc;
    }
    threadIds.push_back(tid);
  }

  return 0;
}



/* ****************************************************************************
*
* QueueWorkers::stop() -
*/
int QueueWorkers::stop()
{
  // Put in the queue as many kill messages as threads we have
  for (unsigned int ix = 0; ix < threadIds.size(); ++ix)
  {
    SenderThreadParams*  paramsP = new SenderThreadParams();
    paramsP->type = QUEUE_MSG_KILL;

    if (!pQueue->try_push(paramsP, true))
    {
      LM_E(("Runtime Error (thread kill message cannot be sent due to push in queue failed)"));
    }
  }

  // Next, wait for every thread termination
  for (unsigned int ix = 0; ix < threadIds.size(); ++ix)
  {
    pthread_join(threadIds[ix], NULL);
    LM_T(LmtThreadpool, ("Thread %x joined", threadIds[ix]));
  }

  return 0;
}



/* ****************************************************************************
*
* workerFunc -
*/
static void* workerFunc(void* pSyncQ)
{
  SyncQOverflow<SenderThreadParams*>*  queue = (SyncQOverflow<SenderThreadParams*> *) pSyncQ;
  CURL*                                curl;

  // Initialize curl context
  curl = curl_easy_init();

  if (curl == NULL)
  {
    LM_E(("Runtime Error (curl_easy_init)"));
    pthread_exit(NULL);
  }

  for (;;)
  {
    SenderThreadParams* paramsP = queue->pop();

    // The "protocol" to signal thread termination is to find a kill msg
    if (paramsP->type == QUEUE_MSG_KILL)
    {
      LM_T(LmtThreadpool, ("Thread %x receiving termination signal...", pthread_self()));

      delete paramsP;
      curl_easy_cleanup((CURL*) curl);

      pthread_exit(NULL);
    }

    // process paramV to send notification (freeing memory after use)
    doNotify(paramsP, curl, queue, "worker");

    // Reset curl for next iteration
    curl_easy_reset(curl);
  }
}
