/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/alarmMgr.h"
#include "logSummary/logSummary.h"



/* ****************************************************************************
*
* Global vars
*/
static sem_t               lsSem;
static unsigned long long  transactionCounter           = 0;
static unsigned long long  transactionsSinceLastSummary = 0;
static bool                logSummaryOn                 = false;



/* ****************************************************************************
*
* transactionCounterIncrement - 
*/
void transactionCounterIncrement(void)
{
  if (logSummaryOn == false)
  {
    return;
  }

  sem_wait(&lsSem);
  ++transactionCounter;
  ++transactionsSinceLastSummary;
  sem_post(&lsSem);
}



/* ****************************************************************************
*
* logSummary - 
*/
static void* logSummary(void* vP)
{
  int period = (long long) vP;

  while (1)
  {
    long long tCounter;
    long long tSinceLastSummary;

    sleep(period);

    //
    // To minimize the time that the logSummary semaphore is used, the values of the
    // counters are copied to local variables and the 'volatile' counter is reset.
    // With this done, the logSummarysemaphore can be released. We have all the info we need.
    //
    sem_wait(&lsSem);
    tCounter          = transactionCounter;
    tSinceLastSummary = transactionsSinceLastSummary;
    transactionsSinceLastSummary = 0;
    sem_post(&lsSem);


    //
    // Same idea with the alarm manager semaphore.
    // Take sem, get data, release sem.
    // After that - without sem, use the data
    //
    alarmMgr.semTake();

    bool       deActive;         // de: Database Error
    long long  deRaised;
    long long  deReleased;
    long long  neActive;         // ne: Notification Error
    long long  neRaised;
    long long  neReleased;
    long long  biActive;         // bi:  Bad Input
    long long  biRaised;
    long long  biReleased;

    alarmMgr.dbErrorsGet(&deActive, &deRaised, &deReleased);
    alarmMgr.badInputGet(&biActive, &biRaised, &biReleased);
    alarmMgr.notificationErrorGet(&neActive, &neRaised, &neReleased);

    alarmMgr.semGive();

    LM_S(("Transactions: %lu (new: %lu)", tCounter, tSinceLastSummary));
    LM_S(("DB status: %s (created: %d, removed: %d)", deActive? "erroneous" : "ok", deRaised, deReleased));
    LM_S(("Notification failure active alarms: %d (created: %d, removed: %d)", neActive, neRaised, neReleased));
    LM_S(("Bad input active alarms: %d (created: %d, removed: %d)", biActive, biRaised, biReleased));
  }

  return NULL;
}



/* ****************************************************************************
*
* logSummaryInit - 
*/
int logSummaryInit(int period)
{
  logSummaryOn = true;

  // 1. Initialize semaphore for transaction counters
  if (sem_init(&lsSem, 0, 1) == -1)
  {
    LM_E(("Runtime Error (error initializing 'log summary' semaphore: %s)", strerror(errno)));
    return -1;
  }

  if (period == 0)
  {
    return 0;
  }

  // 2. Start the log summary thread
  pthread_t  tid;
  int        ret;
  ret = pthread_create(&tid, NULL, logSummary, (void*) period);
  if (ret != 0)
  {
    LM_E(("Runtime Error (error creating thread: %d)", ret));
    return -2;
  }
  pthread_detach(tid);

  return 0;
}
