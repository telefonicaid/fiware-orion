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
#include "ngsiNotify/ServiceQueue.h"


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
  QueueNotifier(size_t                           defaultQueueSize,
                int                              defaultNumThreads,
                const std::vector<std::string>&  serviceV,
                const std::vector<int>&          serviceQueueSizeV,
                const std::vector<int>&          serviceNumThreadV);
  ~QueueNotifier(void);

  void sendNotifyContextRequest(NotifyContextRequest&            ncr,
                                const ngsiv2::Notification&      notification,
                                const std::string&               tenant,
                                const std::string&               xauthToken,
                                const std::string&               fiwareCorrelator,
                                unsigned int                     correlatorCounter,
                                RenderFormat                     renderFormat,
                                const std::vector<std::string>&  attrsFilter,
                                bool                             blacklist,
                                const std::vector<std::string>&  metadataFilter);
  int start();
  size_t queueSize(const std::string& service);
  void release();

private:
 ServiceQueue                          defaultSq;
 std::map<std::string, ServiceQueue*>  serviceSq;

};

#endif  // SRC_LIB_NGSINOTIFY_QUEUENOTIFIER_H_
