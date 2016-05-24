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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsiNotify/QueueStatistics.h"
#include "ngsiNotify/QueueNotifier.h"



/* ****************************************************************************
*
* QueueNotifier::Notifier -
*/
QueueNotifier::QueueNotifier(size_t queueSize, int numThreads): queue(queueSize), workers(&queue, numThreads)
{
  LM_T(LmtNotifier,("Setting up queue and threads for notifications"));
}



/* ****************************************************************************
*
* QueueNotifier::start -
*/
int QueueNotifier::start()
{
  return workers.start();
}



/* ****************************************************************************
*
* QueueNotifier::sendNotifyContextRequest -
*/
void QueueNotifier::sendNotifyContextRequest
(NotifyContextRequest*            ncr,
  const std::string&               url,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsOrder,
  bool blacklist
)
{
  ConnectionInfo ci;

  //
  // FIXME P5: analyze how much of the code of this function is the same as in Notifier::sendNotifyContextRequest
  // and could be refactored to common functions
  //

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

  //
  // FIXME P8: the stuff about atLeastOneNotDefault was added after PR #729, which makes "/" the default servicePath in
  // request not having that header. However, this causes as side-effect that a
  // "Fiware-ServicePath: /" or "Fiware-ServicePath: /,/" header is added in notifications, thus breaking several tests harness.
  // Given that the "clean" implementation of Fiware-ServicePath propagation will be implemented
  // soon (it has been scheduled for version 0.19.0, see https://github.com/telefonicaid/fiware-orion/issues/714)
  // we introduce the atLeastOneNotDefault hack. Once #714 gets implemented,
  // this FIXME will be removed (and all the test harness adjusted, if needed)
  //
  if (!atLeastOneNotDefault)
  {
    spathList = "";
  }

  ci.outMimeType = JSON;
  std::string payload = ncr->toJson(&ci, renderFormat, attrsOrder, blacklist);

  /* Parse URL */
  std::string  host;
  int          port;
  std::string  uriPath;
  std::string  protocol;

  if (!parseUrl(url, host, port, uriPath, protocol))
  {
    std::string details = std::string("sending NotifyContextRequest: malformed URL: '") + url + "'";
    alarmMgr.badInput(clientIp, details);

    return;
  }

  /* Set Content-Type */
  std::string content_type = "application/json";

  SenderThreadParams* params = new SenderThreadParams();
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
  params->mimeType         = JSON;
  params->renderFormat     = renderFormatToString(renderFormat, false);
  params->fiwareCorrelator = fiwareCorrelator;
  strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

  clock_gettime(CLOCK_REALTIME, &params->timeStamp);

  bool enqueued = queue.try_push(params);
  if (!enqueued)
  {
   QueueStatistics::incReject();

   LM_E(("Runtime Error (notification queue is full)"));
   delete params;

   return;
  }

  QueueStatistics::incIn();
}
