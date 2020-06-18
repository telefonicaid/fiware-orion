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
    std::string         url;

    snprintf(portV, sizeof(portV), "%d", params->port);
    url = params->ip + ":" + portV + params->resource;

    strncpy(transactionId, params->transactionId, sizeof(transactionId));

    LM_T(LmtNotifier, ("sending to: host='%s', port=%d, verb=%s, tenant='%s', service-path: '%s', xauthToken: '%s', path='%s', content-type: %s",
                       params->ip.c_str(),
                       params->port,
                       params->verb.c_str(),
                       params->tenant.c_str(),
                       params->servicePath.c_str(),
                       params->xauthToken.c_str(),
                       params->resource.c_str(),
                       params->content_type.c_str()));

    if (!simulatedNotification)
    {
      std::string  out;
      int          r;

      r = httpRequestSend(params->ip,
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

      if (params->toFree != NULL)
      {
        free(params->toFree);
        params->toFree = NULL;
      }

      if (r == 0)
      {
        statisticsUpdate(NotifyContextSent, params->mimeType);
        alarmMgr.notificationErrorReset(url);

        if (params->registration == false)
        {
          subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 0);
        }
      }
      else
      {
        if (params->registration == false)
        {
          subCacheItemNotificationErrorStatus(params->tenant, params->subscriptionId, 1);
        }
      }
    }
    else
    {
      LM_T(LmtNotifier, ("simulatedNotification is 'true', skipping outgoing request"));
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
