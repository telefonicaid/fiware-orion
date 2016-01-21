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
#include <limits.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/alarmMgr.h"
#include "logSummary/logSummary.h"



/* ****************************************************************************
*
* Global vars
*/
static long long  transactionsAtLastSummary  = 0;
static bool       logSummaryOn               = false;



/* ****************************************************************************
*
* logSummary - 
*/
static void* logSummary(void* vP)
{
  int period = *((int*) vP);

  while (1)
  {
    long long transactionsNow;
    long long diff;

    sleep(period);

    transactionsNow     = transactionIdGet();

    // Has transactionId gone 'round-the-corner'?
    if (transactionsNow < transactionsAtLastSummary)
    {
      diff = INT_MAX - transactionsAtLastSummary + transactionsNow;
    }
    else
    {
      diff = transactionsNow - transactionsAtLastSummary;
    }

    // Remember the current transactionId for next loop
    transactionsAtLastSummary = transactionsNow;


    //
    // To minimize the time that the alarm manager semaphore is used, the values of the
    // counters are copied to local variables.
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

    LM_S(("Transactions: %lu (new: %lu)", transactionsNow, diff));
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
int logSummaryInit(int* periodP)
{
  //
  // If the period is ZERO, then the log summary is turned OFF
  //
  if (*periodP == 0)
  {
    return 0;
  }

  //
  // Start the log summary thread
  //
  pthread_t  tid;
  int        ret;
  ret = pthread_create(&tid, NULL, logSummary, (void*) periodP);
  if (ret != 0)
  {
    LM_E(("Runtime Error (error creating thread: %d)", ret));
    return -2;
  }
  pthread_detach(tid);

  logSummaryOn = true;

  return 0;
}
