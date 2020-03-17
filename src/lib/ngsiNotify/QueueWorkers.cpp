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

#include "common/clockFunctions.h"
#include "common/statistics.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"
#include "cache/subCache.h"
#include "ngsi10/NotifyContextRequest.h"
#include "rest/httpRequestSend.h"
#include "ngsiNotify/QueueStatistics.h"
#include "orionld/common/orionldState.h"
#include "orionld/mqtt/mqttNotification.h"
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

  // Initialize kjson and kalloc libs - needed for MQTT
  orionldStateInit();
  orionldState.kjson.spacesPerIndent    = 0;
  orionldState.kjson.stringBeforeColon  = (char*) "";
  orionldState.kjson.stringAfterColon   = (char*) "";
  orionldState.kjson.nlString           = (char*) "";

  for (;;)
  {
    std::vector<SenderThreadParams*>* paramsV = queue->pop();

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

      LM_T(LmtNotifier, ("worker sending '%s' message to: host='%s', port=%d, verb=%s, tenant='%s', service-path: '%s', xauthToken: '%s', path='%s', content-type: %s",
                         params->protocol.c_str(),
                         params->ip.c_str(),
                         params->port,
                         params->verb.c_str(),
                         params->tenant.c_str(),
                         params->servicePath.c_str(),
                         params->xauthToken.c_str(),
                         params->resource.c_str(),
                         params->content_type.c_str()));

      int r = 0;

      if (simulatedNotification)
      {
        LM_T(LmtNotifier, ("simulatedNotification is 'true', skipping outgoing request"));
        __sync_fetch_and_add(&noOfSimulatedNotifications, 1);
      }
      else if (params->protocol == "mqtt:")
      {
        char* topic = (char*) params->resource.c_str();

        ++topic;  // step over the initial '/'
        LM_TMP(("MQTT: Preparing to send an MQTT notification to '%s', port %d, topic '%s'", params->ip.c_str(), params->port, topic));
        LM_TMP(("MQTT: Payload Data: %s", params->content.c_str()));

        r = mqttNotification(params->ip.c_str(), params->port, topic, params->content.c_str(), params->content_type.c_str());
      }
      else // we'll send the notification
      {
        std::string  out;

        r = httpRequestSendWithCurl(curl,
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
                                    true,
                                    NOTIFICATION_WAIT_MODE,
                                    &out,
                                    params->extraHeaders);
      }

      if (!simulatedNotification)
      {
        //
        // FIXME: ok and error counter should be incremented in the other notification modes (generalizing the concept, i.e.
        // not as member of QueueStatistics:: which seems to be tied to just the threadpool notification mode)
        //
        char portV[STRING_SIZE_FOR_INT];
        snprintf(portV, sizeof(portV), "%d", params->port);
        std::string url = params->ip + ":" + portV + params->resource;

        if (r == 0)
        {
          statisticsUpdate(NotifyContextSent, params->mimeType);
          QueueStatistics::incSentOK();
          alarmMgr.notificationErrorReset(url);

          if (params->registration == false)
          {
            subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 0);
          }
        }
        else
        {
          QueueStatistics::incSentError();
          alarmMgr.notificationError(url, "notification failure for queue worker");

          if (params->registration == false)
          {
            subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 1);
          }
        }
      }

      // Free params memory
      delete params;
    }

    // Free params vector memory
    delete paramsV;

    // Reset curl for next iteration
    curl_easy_reset(curl);
  }
}
