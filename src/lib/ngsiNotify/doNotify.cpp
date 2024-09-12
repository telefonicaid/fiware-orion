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

#include <curl/curl.h>

#include "ngsiNotify/doNotify.h"

#include "ngsiNotify/senderThread.h"
#include "ngsiNotify/QueueStatistics.h"

#include "alarmMgr/alarmMgr.h"
#include "mqtt/mqttMgr.h"
#include "rest/httpRequestSend.h"
#include "cache/subCache.h"

#include "logMsg/logMsg.h"

#include "common/clockFunctions.h"
#include "common/SyncQOverflow.h"
#include "common/limits.h"
#include "common/statistics.h"
#include "common/logTracing.h"



/* ****************************************************************************
*
* doNotifyHttp -
*
*/
static void doNotifyHttp(SenderThreadParams* params, CURL* curl, SyncQOverflow<SenderThreadParams*>*  queue)
{
  long long    statusCode = -1;
  std::string  out;

  char                portV[STRING_SIZE_FOR_INT];
  std::string         endpoint;
  std::string         url;

  snprintf(portV, sizeof(portV), "%d", params->port);
  endpoint = params->ip + ":" + portV;
  url = endpoint + params->resource;

  int r =  httpRequestSend(curl,
                       "subId: " + params->subscriptionId,
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

  LM_T(LmtNotificationResponsePayload, ("notification response: %s", out.c_str()));

  //
  // FIXME: ok and error counter should be incremented in the other notification modes (generalizing the concept, i.e.
  // not as member of QueueStatistics:: which seems to be tied to just the threadpool notification mode)
  //
  if (r == 0)
  {
    if (queue != NULL)
    {
      // queue statistics collected only if queue is not null
      QueueStatistics::incSentOK();
    }
    __sync_fetch_and_add(&noOfNotificationsSent, 1);
    alarmMgr.notificationErrorReset(url);

    // FIXME P3: at this point probably this is always false and the if condition could be removed,
    // but it's safer this way...
    if (params->registration == false)
    {
      subNotificationErrorStatus(params->tenant, params->subscriptionId, false, statusCode, "");
    }
  }
  else
  {
    if (queue != NULL)
    {
      // queue statistics collected only if queue is not null
      QueueStatistics::incSentError();
    }
    alarmMgr.notificationError(url, "notification failure for queue worker: " + out);

    // FIXME P3: at this point probably this is always false and the if condition could be removed,
    // but it's safer this way...
    if (params->registration == false)
    {
      subNotificationErrorStatus(params->tenant, params->subscriptionId, true, -1, out, params->failsCounter, params->maxFailsLimit);
    }
  }

  // Add notificacion result summary in log INFO level
  if (statusCode != -1)
  {
    logInfoHttpNotification(params->subscriptionId.c_str(), endpoint.c_str(), params->verb.c_str(), params->resource.c_str(), params->content.c_str(), statusCode);
  }
  else
  {
    logInfoHttpNotification(params->subscriptionId.c_str(), endpoint.c_str(), params->verb.c_str(), params->resource.c_str(), params->content.c_str(), out.c_str());
  }
}



/* ****************************************************************************
*
* doNotifyMqtt -
*
*/
static void doNotifyMqtt(SenderThreadParams* params)
{
  char                portV[STRING_SIZE_FOR_INT];
  std::string         endpoint;

  snprintf(portV, sizeof(portV), "%d", params->port);
  endpoint = params->ip + ":" + portV;

  // Note in the case of HTTP this lmTransactionStart is done internally in httpRequestSend
  std::string protocol = params->protocol + "//";
  correlatorIdSet(params->fiwareCorrelator.c_str());
  lmTransactionStart("to", protocol.c_str(), + params->ip.c_str(), params->port, params->resource.c_str(),
                     params->tenant.c_str(), params->servicePath.c_str(), params->from.c_str());

  // Note that we use in subNotificationErrorStatus() statusCode -1 and failureReson "" to avoid using
  // lastFailureReason and lastSuccessCode in MQTT notifications (they don't have sense in this case)
  if (mqttMgr.sendMqttNotification(params->ip, params->port, params->user, params->passwd, params->content, params->resource, params->qos, params->retain))
  {
    // MQTT transaction is logged only in the case it was actually published. Upon successful publishing
    // mqttOnPublishCallback is called (by the moment we are not doing nothing there, just printing in
    // DEBUG log level). Note however that even if mqttOnPublishCallback() is called there is no actual
    // guarantee if MQTT QoS is 0
    logInfoMqttNotification(params->subscriptionId.c_str(), endpoint.c_str(), params->resource.c_str(), params->content.c_str());
    subNotificationErrorStatus(params->tenant, params->subscriptionId, false, -1, "");
  }
  else
  {
    subNotificationErrorStatus(params->tenant, params->subscriptionId, true, -1, "", params->failsCounter, params->maxFailsLimit);
  }
}



/* ****************************************************************************
*
* doNotify -
*
* Note this function "consumes" paramsV, freeing its allocated dynamic memory
*/
void doNotify
(
  SenderThreadParams*                  paramsP,
  CURL*                                curl,
  SyncQOverflow<SenderThreadParams*>*  queue,
  const char*                          logPrefix
)
{
  if (queue != NULL)
  {
    struct timespec     now;
    struct timespec     howlong;
    size_t              estimatedQSize;

    QueueStatistics::incOut();
    clock_gettime(CLOCK_REALTIME, &now);
    clock_difftime(&now, &paramsP->timeStamp, &howlong);
    estimatedQSize = queue->size();
    QueueStatistics::addTimeInQWithSize(&howlong, estimatedQSize);
  }

  strncpy(transactionId, paramsP->transactionId, sizeof(transactionId));

  LM_T(LmtNotifier, ("%s sending to: host='%s', port=%d, verb=%s, tenant='%s', service-path: '%s', xauthToken: '%s', resource='%s', content-type: %s, qos=%d, timeout=%d, user=%s, passwd=*****, maxFailsLimit=%lu, failsCounter=%lu",
                     logPrefix,
                     paramsP->ip.c_str(),
                     paramsP->port,
                     paramsP->verb.c_str(),
                     paramsP->tenant.c_str(),
                     paramsP->servicePath.c_str(),
                     paramsP->xauthToken.c_str(),
                     paramsP->resource.c_str(),
                     paramsP->content_type.c_str(),
                     paramsP->qos,
                     paramsP->timeout,
                     paramsP->user.c_str(),
                     paramsP->maxFailsLimit,
                     paramsP->failsCounter));

  LM_T(LmtNotificationRequestPayload , ("notification request payload: %s", paramsP->content.c_str()));

  if (simulatedNotification)
  {
    LM_T(LmtNotifier, ("simulatedNotification is 'true', skipping outgoing request"));
    __sync_fetch_and_add(&noOfSimulatedNotifications, 1);
  }
  else // we'll send the notification
  {
    if (paramsP->protocol == "mqtt:")
    {
      doNotifyMqtt(paramsP);
    }
    else
    {
      doNotifyHttp(paramsP, curl, queue);
    }
  }

  // End transaction
  lmTransactionEnd();

  // Free params memory
  delete paramsP;
}
