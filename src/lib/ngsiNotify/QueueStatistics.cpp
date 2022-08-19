/*
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

#include <stdio.h>

#include "common/clockFunctions.h"

#include "ngsiNotify/QueueStatistics.h"

// This implementation could be 'improved' when boost >=1.53.0 || C++ 11
// both give atomic types

volatile int QueueStatistics::noOfNotificationsQueueIn;
volatile int QueueStatistics::noOfNotificationsQueueOut;
volatile int QueueStatistics::noOfNotificationsQueueReject;
volatile int QueueStatistics::noOfNotificationsQueueSentOK;
volatile int QueueStatistics::noOfNotificationsQueueSentError;

boost::mutex QueueStatistics::mtxTimeInQ;
struct timespec QueueStatistics::timeInQ;
size_t QueueStatistics::queueSize;

/* ****************************************************************************
*
* getIn -
*/
int  QueueStatistics::getIn()
{
  return __sync_fetch_and_add(&noOfNotificationsQueueIn, 0);
}

/* ****************************************************************************
*
* incIn -
*/
void QueueStatistics::incIn(int n)
{
  __sync_fetch_and_add(&noOfNotificationsQueueIn, n);
}

/* ****************************************************************************
*
* getOut -
*/
int  QueueStatistics::getOut()
{
  return __sync_fetch_and_add(&noOfNotificationsQueueOut, 0);
}

/* ****************************************************************************
*
* incOut -
*/
void QueueStatistics::incOut(int n)
{
  __sync_fetch_and_add(&noOfNotificationsQueueOut, n);
}

/* ****************************************************************************
*
* getReject -
*/
int  QueueStatistics::getReject()
{
  return __sync_fetch_and_add(&noOfNotificationsQueueReject, 0);
}

/* ****************************************************************************
*
* incReject -
*/
void QueueStatistics::incReject(int n)
{
  __sync_fetch_and_add(&noOfNotificationsQueueReject ,n);
}

/* ****************************************************************************
*
* getSentOK -
*/
int  QueueStatistics::getSentOK()
{
  return __sync_fetch_and_add(&noOfNotificationsQueueSentOK, 0);
}

/* ****************************************************************************
*
* incSentOK -
*/
void QueueStatistics::incSentOK()
{
  __sync_fetch_and_add(&noOfNotificationsQueueSentOK, 1);
}

/* ****************************************************************************
*
* getSentError -
*/
int  QueueStatistics::getSentError()
{
  return __sync_fetch_and_add(&noOfNotificationsQueueSentError, 0);
}

/* ****************************************************************************
*
* incSentError -
*/
void QueueStatistics::incSentError()
{
  __sync_fetch_and_add(&noOfNotificationsQueueSentError, 1);
}
/* ****************************************************************************
*
* getTimInQ -
*/
float QueueStatistics::getTimeInQ(void)
{
  boost::mutex::scoped_lock lock(mtxTimeInQ);

  // As the others time statistics
  return timeInQ.tv_sec + ((float) timeInQ.tv_nsec ) / 1E9;
}

/* ****************************************************************************
*
* addTimInQ -
*/
void QueueStatistics::addTimeInQWithSize(const struct timespec* diff, size_t qSize)
{
  boost::mutex::scoped_lock lock(mtxTimeInQ);

  queueSize = qSize;
  clock_addtime(&timeInQ, diff);

}

/* ****************************************************************************
*
* getQSize() -
*/
size_t QueueStatistics::getQSize()
{
  boost::mutex::scoped_lock lock(mtxTimeInQ);

  return  queueSize;
}

/* ****************************************************************************
*
* reset() -
*/
void QueueStatistics::reset()
{
  __sync_fetch_and_and(&noOfNotificationsQueueIn, 0);
  __sync_fetch_and_and(&noOfNotificationsQueueOut, 0);
  __sync_fetch_and_and(&noOfNotificationsQueueReject, 0);
  __sync_fetch_and_and(&noOfNotificationsQueueSentOK, 0);
  __sync_fetch_and_and(&noOfNotificationsQueueSentError, 0);

  boost::mutex::scoped_lock lock(mtxTimeInQ);
  timeInQ.tv_sec = 0;
  timeInQ.tv_nsec = 0;
}
