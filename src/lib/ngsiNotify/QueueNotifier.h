#ifndef SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H_
#define SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H_

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

#include "common/SyncQOverflow.h"
#include "common/RenderFormat.h"
#include "ngsiNotify/Notifier.h"
#include "ngsiNotify/senderThread.h"
#include "ngsiNotify/QueueWorkers.h"


// default queue size
#define DEFAULT_NOTIF_QS 100
// default number of threads
#define DEFAULT_NOTIF_TN 10



/* ****************************************************************************
*
* class QueueNotifier -
*/
class QueueNotifier : public Notifier
{
public:
  QueueNotifier(size_t queueSize, int numThreads);

  void sendNotifyContextRequest(NotifyContextRequest*            ncr,
                                const ngsiv2::HttpInfo&          httpInfo,
                                const std::string&               tenant,
                                const std::string&               xauthToken,
                                const std::string&               fiwareCorrelator,
                                RenderFormat                     renderFormat,
                                const std::vector<std::string>&  attrsOrder,
                                const std::vector<std::string>&  metadataFilter,
                                bool                             blacklist);
  int start();

private:
 SyncQOverflow<std::vector<SenderThreadParams*>*>  queue;
 QueueWorkers                        workers;

};

#endif  // SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H_
