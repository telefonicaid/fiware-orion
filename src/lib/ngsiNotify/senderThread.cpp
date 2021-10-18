/* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán Márquez
*/
#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/statistics.h"
#include "common/limits.h"
#include "common/globals.h"
#include "common/logTracing.h"
#include "alarmMgr/alarmMgr.h"
#include "mqtt/mqttMgr.h"
#include "rest/httpRequestSend.h"
#include "ngsiNotify/senderThread.h"
#include "cache/subCache.h"

/* ****************************************************************************
*
* startSenderThread -
*/
void* startSenderThread(void* p)
{
  std::vector<SenderThreadParams*>* paramsV = (std::vector<SenderThreadParams*>*) p;

  for (unsigned ix = 0; ix < paramsV->size(); ix++)
  {
    SenderThreadParams* params = (SenderThreadParams*) (*paramsV)[ix];
    char                portV[STRING_SIZE_FOR_INT];
    std::string         endpoint;
    std::string         url;

    snprintf(portV, sizeof(portV), "%d", params->port);
    endpoint = params->ip + ":" + portV;
    url = endpoint + params->resource;

    strncpy(transactionId, params->transactionId, sizeof(transactionId));

    LM_T(LmtNotifier, ("sending to: host='%s', port=%d, verb=%s, tenant='%s', service-path: '%s', xauthToken: '%s', resource='%s', content-type: %s, qos=%d, timeout=%d, user=%s, passwd=*****",
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

    long long    statusCode = -1;
    std::string  out;
    LM_T(LmtNotificationRequestPayload , ("notification request payload: %s", params->content.c_str()));

    if (!simulatedNotification)
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
        r = httpRequestSend(params->from,
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
      }

      if (r == 0)
      {
        __sync_fetch_and_add(&noOfNotificationsSent, 1);
        alarmMgr.notificationErrorReset(url);

        if (params->registration == false)
        {
          subNotificationErrorStatus(params->tenant, params->subscriptionId, 0, statusCode, "");
        }
      }
      else
      {
        alarmMgr.notificationError(url, "notification failure for sender-thread: " + out);

        if (params->registration == false)
        {
          subNotificationErrorStatus(params->tenant, params->subscriptionId, -1, -1, out);
        }
      }
    }
    else
    {
      LM_T(LmtNotifier, ("simulatedNotification is 'true', skipping outgoing request"));
      __sync_fetch_and_add(&noOfSimulatedNotifications, 1);
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

    /* Delete the parameters after using them */
    delete params;
  }

  /* Delete the parameters vector after using it */
  delete paramsV;

  pthread_exit(NULL);
  return NULL;
}
