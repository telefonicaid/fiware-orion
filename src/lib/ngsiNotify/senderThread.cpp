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
#include "alarmMgr/alarmMgr.h"
#include "rest/httpRequestSend.h"
#include "ngsiNotify/senderThread.h"
#include "cache/subCache.h"
#include "orionld/common/orionldState.h"
#include "orionld/mqtt/mqttNotification.h"



/* ****************************************************************************
*
* startSenderThread -
*/
void* startSenderThread(void* p)
{
  std::vector<SenderThreadParams*>* paramsV = (std::vector<SenderThreadParams*>*) p;

  // Initialize kjson and kalloc libs - needed for MQTT
  orionldStateInit(NULL);
  orionldState.kjson.spacesPerIndent    = 0;
  orionldState.kjson.stringBeforeColon  = (char*) "";
  orionldState.kjson.stringAfterColon   = (char*) "";
  orionldState.kjson.nlString           = (char*) "";

  for (unsigned ix = 0; ix < paramsV->size(); ix++)
  {
    SenderThreadParams* params = (SenderThreadParams*) (*paramsV)[ix];
    char                portV[STRING_SIZE_FOR_INT];
    std::string         url;

    snprintf(portV, sizeof(portV), "%d", params->port);
    url = params->ip + ":" + portV + params->resource;


    // To avoid buffer size complaints from the compiler
    // Delicate problem, as copies are made in both directions
    // Seems like I have to avoid strncpy here :(
    //
    strcpy(transactionId, params->transactionId);

    if (!simulatedNotification)
    {
      int r;

      if (strncmp(params->protocol.c_str(), "mqtt", 4) != 0)  // Not MQTT, must be HTTP
      {
        std::string  out;

        LM(("Sending HTTP Notification for subscription '%s'", params->subscriptionId.c_str()));
        r = httpRequestSend(params->ip,
                            params->port,
                            params->protocol,
                            params->verb,
                            params->tenant.c_str(),
                            params->servicePath,
                            params->xauthToken.c_str(),
                            params->resource,
                            params->content_type,
                            params->content,
                            params->fiwareCorrelator,
                            params->renderFormat,
                            NOTIFICATION_WAIT_MODE,
                            &out,
                            params->extraHeaders,
                            "",
                            -1,
                            params->subscriptionId.c_str());  // Subscription ID as URL param
      }
      else
      {
        char* topic = (char*) params->resource.c_str();

        LM(("Sending MQTT Notification for subscription '%s'", params->subscriptionId.c_str()));
        r = mqttNotification(params->ip.c_str(),
                             params->port,
                             topic,
                             params->content.c_str(),
                             params->content_type.c_str(),
                             params->mqttQoS,
                             params->mqttUserName,
                             params->mqttPassword,
                             params->mqttVersion,
                             params->xauthToken.c_str(),
                             params->extraHeaders);
      }

      if (params->toFree != NULL)
      {
        free(params->toFree);
        params->toFree = NULL;
      }

      CachedSubscription*  subP               = subCacheItemLookup(params->tenant.c_str(), params->subscriptionId.c_str());
      bool                 ngsildSubscription = ((subP != NULL) && (subP->ldContext != ""))? true : false;

      if (r == 0)
      {
        statisticsUpdate(NotifyContextSent, params->mimeType);
        alarmMgr.notificationErrorReset(url);

        if (params->registration == false)
          subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 0, ngsildSubscription);
      }
      else
      {
        if (params->registration == false)
          subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 1, ngsildSubscription);
      }
    }
    else
    {
      __sync_fetch_and_add(&noOfSimulatedNotifications, 1);
      alarmMgr.notificationError(url, "notification failure for sender-thread");
    }

    /* Delete the parameters after using them */
    delete params;
  }

  /* Delete the parameters vector after using it */
  delete paramsV;

  pthread_exit(NULL);
  return NULL;
}
