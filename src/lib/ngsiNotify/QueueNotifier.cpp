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
* QueueNotifier::QueueNotifier -
*/
QueueNotifier::QueueNotifier(size_t queueSize, int numThreads): defaultSq(queueSize, numThreads)
{
  // FIXME PR: unhardwire this
  serviceSq["serv1"] = new ServiceQueue(1, 2);
  serviceSq["serv2"] = new ServiceQueue(2, 2);
}



/* ****************************************************************************
*
* QueueNotifier::~QueueNotifier -
*/
QueueNotifier::~QueueNotifier(void)
{
  for (std::map<std::string, ServiceQueue*>::const_iterator it = serviceSq.begin(); it != serviceSq.end(); ++it)
  {
    delete it->second;
  }
}



/* ****************************************************************************
*
* QueueNotifier::start -
*/
int QueueNotifier::start()
{
  // exitCode = 0 means everything ok. We sum all the results, so only if all them are
  // ok then the result of this function is ok
  int exitCode = defaultSq.start();
  for (std::map<std::string, ServiceQueue*>::const_iterator it = serviceSq.begin(); it != serviceSq.end(); ++it)
  {
    exitCode += it->second->start();
  }
  return exitCode;
}



/* ****************************************************************************
*
* QueueNotifier::queueSize -
*
*/
size_t QueueNotifier::queueSize()
{
  // FIXME PR: per-service. Adjust flow control documentation
  return defaultSq.size();
}



/* ****************************************************************************
*
* QueueNotifier::sendNotifyContextRequest -
*/
void QueueNotifier::sendNotifyContextRequest
(
  NotifyContextRequest&            ncr,
  const ngsiv2::HttpInfo&          httpInfo,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  unsigned int                     correlatorCounter,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
  std::vector<SenderThreadParams*>* paramsV = Notifier::buildSenderParams(ncr,
                                                                          httpInfo,
                                                                          tenant,
                                                                          xauthToken,
                                                                          fiwareCorrelator,
                                                                          correlatorCounter,
                                                                          renderFormat,
                                                                          attrsFilter,
                                                                          blacklist,
                                                                          metadataFilter);

  size_t notificationsNum = paramsV->size();
  for (unsigned ix = 0; ix < notificationsNum; ix++)
  {
    clock_gettime(CLOCK_REALTIME, &(((*paramsV)[ix])->timeStamp));
  }

  // Try to use per-service queue. If not found, use the default queue
  ServiceQueue* sq;

  std::map<std::string, ServiceQueue*>::iterator iter = serviceSq.find(tenant);
  std::string queueName;
  if (iter != serviceSq.end())
  {
    queueName = tenant;
    sq = iter->second;
  }
  else
  {
    queueName = "default";
    sq = &defaultSq;
  }

  bool enqueued = sq->try_push(paramsV);
  if (!enqueued)
  {
    QueueStatistics::incReject(notificationsNum);
    LM_E(("Runtime Error (%s notification queue is full)", queueName.c_str()));
    for (unsigned ix = 0; ix < paramsV->size(); ix++)
    {
      delete (*paramsV)[ix];
    }
    delete paramsV;

    return;
  }

  QueueStatistics::incIn(notificationsNum);
}
