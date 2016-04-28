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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/statistics.h"
#include "common/limits.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi10/NotifyContextRequest.h"
#include "rest/httpRequestSend.h"
#include "ngsiNotify/senderThread.h"
#include "ngsiNotify/Notifier.h"



/* ****************************************************************************
*
* ~Notifier -
*/
Notifier::~Notifier (void)
{
  // FIXME: This destructor is needed to avoid warning message.
  // Compilation fails when a warning occurs, and it is enabled 
  // compilation option -Werror "warnings being treated as errors" 
  LM_T(LmtNotImplemented, ("Notifier destructor is not implemented"));
}

/* ****************************************************************************
*
* Notifier::sendNotifyContextRequest -
*/
void Notifier::sendNotifyContextRequest
(
  NotifyContextRequest*            ncr,
  const std::string&               url,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsOrder
)
{
    ConnectionInfo ci;

    //
    // Creating the value of the Fiware-ServicePath HTTP header.
    // This is a comma-separated list of the service-paths in the same order as the entities come in the payload
    //
    std::string spathList;
    bool atLeastOneNotDefault = false;
    for (unsigned int ix = 0; ix < ncr->contextElementResponseVector.size(); ++ix)
    {
      EntityId* eP = &ncr->contextElementResponseVector[ix]->contextElement.entityId;

      if (spathList != "")
      {
        spathList += ",";
      }
      spathList += eP->servicePath;
      atLeastOneNotDefault = atLeastOneNotDefault || (eP->servicePath != "/");
    }
    // FIXME P8: the stuff about atLeastOneNotDefault was added after PR #729, which makes "/" the default servicePath in
    // request not having that header. However, this causes as side-effect that a
    // "Fiware-ServicePath: /" or "Fiware-ServicePath: /,/" header is added in notifications, thus breaking several tests harness.
    // Given that the "clean" implementation of Fiware-ServicePath propagation will be implemented
    // soon (it has been scheduled for version 0.19.0, see https://github.com/telefonicaid/fiware-orion/issues/714)
    // we introduce the atLeastOneNotDefault hack. Once #714 gets implemented,
    // this FIXME will be removed (and all the test harness adjusted, if needed)
    if (!atLeastOneNotDefault)
    {
      spathList = "";
    }
    
    ci.outFormat = JSON;

    std::string payload;
    if (renderFormat == NGSI_V1_LEGACY)
    {
      payload = ncr->render(&ci, NotifyContext, "");
    }
    else
    {
      payload = ncr->toJson(&ci, renderFormat, attrsOrder);
    }

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", url.c_str()));
      return;
    }

    /* Set Content-Type */
    std::string content_type = "application/json";

    /* Send the message (no wait for response), in a separate thread to avoid blocking */
    pthread_t            tid;
    SenderThreadParams*  params = new SenderThreadParams();

    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = "POST";
    params->tenant           = tenant;
    params->servicePath      = spathList;
    params->xauthToken       = xauthToken;
    params->resource         = uriPath;
    params->content_type     = content_type;
    params->content          = payload;
    params->format           = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->fiwareCorrelator = fiwareCorrelator;

    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
}



/* ****************************************************************************
*
* Notifier::sendNotifyContextAvailabilityRequest -
*
* FIXME: this method is very similar to sendNotifyContextRequest and probably
* they could be refactored in the future to have a common part using a parent
* class for both types of notifications and using it as first argument
*/
void Notifier::sendNotifyContextAvailabilityRequest
(
  NotifyContextAvailabilityRequest*  ncar,
  const std::string&                 url,
  const std::string&                 tenant,
  const std::string&                 fiwareCorrelator,
  RenderFormat                       renderFormat
)
{
    /* Render NotifyContextAvailabilityRequest */
    std::string payload = ncar->render(NotifyContextAvailability, "");

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      std::string details = std::string("sending NotifyContextAvailabilityRequest: malformed URL: '") + url + "'";
      alarmMgr.badInput(clientIp, details);

      return;
    }

    /* Set Content-Type */
    std::string content_type = "application/json";

    /* Send the message (without awaiting response, in a separate thread to avoid blocking) */
    pthread_t            tid;
    SenderThreadParams*  params = new SenderThreadParams();

    params->ip               = host;
    params->port             = port;
    params->verb             = "POST";
    params->tenant           = tenant;
    params->resource         = uriPath;   
    params->content_type     = content_type;
    params->content          = payload;
    params->fiwareCorrelator = fiwareCorrelator;
    params->renderFormat     = renderFormatToString(renderFormat);

    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
}
