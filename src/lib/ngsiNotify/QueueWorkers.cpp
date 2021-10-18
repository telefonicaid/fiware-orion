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

#include "common/globals.h"
#include "common/clockFunctions.h"
#include "common/statistics.h"
#include "common/limits.h"
#include "common/logTracing.h"
#include "alarmMgr/alarmMgr.h"
#include "mqtt/mqttMgr.h"

#include "cache/subCache.h"
#include "ngsi10/NotifyContextRequest.h"
#include "rest/httpRequestSend.h"
#include "ngsiNotify/QueueStatistics.h"
#include "ngsiNotify/QueueWorkers.h"



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
      LM_E(("Internal Error (pthread_create: %s)", strerror(errno)));
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
    std::vector<SenderThreadParams*>* paramsV = new std::vector<SenderThreadParams*>();

    SenderThreadParams*  params = new SenderThreadParams();
    params->type = QUEUE_MSG_KILL;
    paramsV->push_back(params);

    if (!pQueue->try_push(paramsV, true))
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
  SyncQOverflow<std::vector<SenderThreadParams*>*>*  queue = (SyncQOverflow<std::vector<SenderThreadParams*>*> *) pSyncQ;
  CURL*                                              curl;

  // Initialize curl context
  curl = curl_easy_init();

  if (curl == NULL)
  {
    LM_E(("Runtime Error (curl_easy_init)"));
    pthread_exit(NULL);
  }

  for (;;)
  {
    std::vector<SenderThreadParams*>* paramsV = queue->pop();

    // The "protocol" to signal thread termination is to find a kill msg in the first
    // element of the paramsV vector (note that by construction in QueueWorkers::stop()
    // this vector will have only one item in this case)
    if ((paramsV->size() == 1) && ((*paramsV)[0]->type == QUEUE_MSG_KILL))
    {
      LM_T(LmtThreadpool, ("Thread %x receiving termination signal...", pthread_self()));

      delete (*paramsV)[0];
      delete paramsV;
      curl_easy_cleanup((CURL*) curl);

      pthread_exit(NULL);
    }

    for (unsigned ix = 0; ix < paramsV->size(); ix++)
    {
      struct timespec     now;
      struct timespec     howlong;
      size_t              estimatedQSize;

      SenderThreadParams* params = (*paramsV)[ix];

      QueueStatistics::incOut();
      clock_gettime(CLOCK_REALTIME, &now);
      clock_difftime(&now, &params->timeStamp, &howlong);
      estimatedQSize = queue->size();
      QueueStatistics::addTimeInQWithSize(&howlong, estimatedQSize);

      strncpy(transactionId, params->transactionId, sizeof(transactionId));

      LM_T(LmtNotifier, ("worker sending to: host='%s', port=%d, verb=%s, tenant='%s', service-path: '%s', xauthToken: '%s', resource='%s', content-type: %s, qos=%d, timeout=%d, user=%s, passwd=*****",
                         params->ip.c_str(),
                         params->port,
                         params->verb.c_str(),
                         params->tenant.c_str(),
                         params->servicePath.c_str(),
                         params->xauthToken.c_str(),
                         params->resource.c_str(),
                         params->content_type.c_str(),
                         params->qos,
                         params->timeout,
                         params->user.c_str()));

      char                portV[STRING_SIZE_FOR_INT];
      std::string         endpoint;
      std::string         url;

      snprintf(portV, sizeof(portV), "%d", params->port);
      endpoint = params->ip + ":" + portV;
      url = endpoint + params->resource;

      long long    statusCode = -1;
      std::string  out;

      if (simulatedNotification)
      {
        LM_T(LmtNotifier, ("simulatedNotification is 'true', skipping outgoing request"));
        __sync_fetch_and_add(&noOfSimulatedNotifications, 1);
      }
      else // we'll send the notification
      {
        int  r;

        if (params->protocol == "mqtt:")
        {
          // Note in the case of HTTP this lmTransactionStart is done internally in httpRequestSend
          std::string protocol = params->protocol + "//";
          correlatorIdSet(params->fiwareCorrelator.c_str());
          lmTransactionStart("to", protocol.c_str(), + params->ip.c_str(), params->port, params->resource.c_str(),
                             params->tenant.c_str(), params->servicePath.c_str(), params->from.c_str());

          mqttMgr.sendMqttNotification(params->ip, params->port, params->user, params->passwd, params->content, params->resource, params->qos);

          // In MQTT notifications we don't have any response, so we always assume they are ok
          // When publish is sucessfull mqttOnPublishCallback is called (by the moment we are not doing nothing
          // there, just printing in DEBUG log level)
          r = 0;
        }
        else
        {
          r =  httpRequestSendWithCurl(curl,
                                       params->from,
                                       params->ip,
                                       params->port,
                                       params->protocol,
                                       params->verb,
                                       params->tenant,
                                       params->servicePath,
                                       params->xauthToken,
                                       params->resource,
                                       params->content_type,
                                       params->content,
                                       params->fiwareCorrelator,
                                       params->renderFormat,
                                       &out,
                                       &statusCode,
                                       params->extraHeaders,
                                       "",                         //default acceptFormat
                                       params->timeout);
        }

        //
        // FIXME: ok and error counter should be incremented in the other notification modes (generalizing the concept, i.e.
        // not as member of QueueStatistics:: which seems to be tied to just the threadpool notification mode)
        //
        if (r == 0)
        {
          __sync_fetch_and_add(&noOfNotificationsSent, 1);
          QueueStatistics::incSentOK();
          alarmMgr.notificationErrorReset(url);

          if (params->registration == false)
          {
            subNotificationErrorStatus(params->tenant, params->subscriptionId, 0, statusCode, "");
          }
        }
        else
        {
          QueueStatistics::incSentError();
          alarmMgr.notificationError(url, "notification failure for queue worker: " + out);

          if (params->registration == false)
          {
            subNotificationErrorStatus(params->tenant, params->subscriptionId, 1, -1, out);
          }
        }
      }

      // Add notificacion result summary in log INFO level
      if (statusCode != -1)
      {
        logInfoNotification(params->subscriptionId.c_str(), params->protocol.c_str(), endpoint.c_str(), params->verb.c_str(), params->resource.c_str(), statusCode);
      }
      else
      {
        logInfoNotification(params->subscriptionId.c_str(), params->protocol.c_str(), endpoint.c_str(), params->verb.c_str(), params->resource.c_str(), out.c_str());
      }

      // End transaction
      lmTransactionEnd();

      // Free params memory
      delete params;
    }

    // Free params vector memory
    delete paramsV;

    // Reset curl for next iteration
    curl_easy_reset(curl);
  }
}
