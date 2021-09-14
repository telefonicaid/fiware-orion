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

#include "ServiceQueue.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"


/* ****************************************************************************
*
* ServiceQueue::ServiceQueue -
*/
ServiceQueue::ServiceQueue(size_t queueSize, int numThreads): queue(queueSize), workers(&queue, numThreads)
{
  LM_T(LmtNotifier,("Setting up queue and threads for notifications"));
}



/* ****************************************************************************
*
* ServiceQueue::start -
*/
int ServiceQueue::start(void)
{
  return workers.start();
}



/* ****************************************************************************
*
* ServiceQueue::stop -
*/
int ServiceQueue::stop(void)
{
  return workers.stop();
}


/* ****************************************************************************
*
* ServiceQueue::try_push -
*/
bool ServiceQueue::try_push(std::vector<SenderThreadParams*>* item)
{
  return queue.try_push(item);
}



/* ****************************************************************************
*
* ServiceQueue::size -
*/
size_t ServiceQueue::size() const
{
  return queue.size();
}
