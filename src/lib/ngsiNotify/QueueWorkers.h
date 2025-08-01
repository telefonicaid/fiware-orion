#ifndef SRC_LIB_NGSINOTIFY_QUEUEWORKERS_H_
#define SRC_LIB_NGSINOTIFY_QUEUEWORKERS_H_
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

#include "common/SyncQOverflow.h"
#include "ngsiNotify/senderThread.h"

class QueueWorkers
{
public:
  QueueWorkers(SyncQOverflow<SenderThreadParams*> *pQ, int numThreads): pQueue(pQ), numberOfThreads(numThreads) {}
  int start();
  int stop();
private:
    SyncQOverflow<SenderThreadParams*> *pQueue;
    int numberOfThreads;
    std::vector<pthread_t> threadIds;
};

#endif  // SRC_LIB_NGSINOTIFY_QUEUEWORKERS_H_
