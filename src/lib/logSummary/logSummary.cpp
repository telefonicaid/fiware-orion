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
#include <sys/types.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/alarmMgr.h"
#include "logSummary/logSummary.h"



/* ****************************************************************************
*
* Global vars
*/
static int64_t  transactionsAtLastSummary  = 0;
static bool     logSummaryOn               = false;
static int64_t  deRaisedInLastSummary      = 0;
static int64_t  deReleasedInLastSummary    = 0;
static int64_t  neRaisedInLastSummary      = 0;
static int64_t  neReleasedInLastSummary    = 0;
static int64_t  biRaisedInLastSummary      = 0;
static int64_t  biReleasedInLastSummary    = 0;



/* ****************************************************************************
*
* logSummary - 
*/
static void* logSummary(void* vP)
{
  int period = *((int*) vP);

  while (1)
  {
    int64_t transactionsNow;
    int64_t diff;

    sleep(period);

    transactionsNow = transactionIdGet();

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

    bool     deActive;         // de: Database Error
    int64_t  deRaised;
    int64_t  deReleased;
    int64_t  neActive;         // ne: Notification Error
    int64_t  neRaised;
    int64_t  neReleased;
    int64_t  biActive;         // bi:  Bad Input
    int64_t  biRaised;
    int64_t  biReleased;

    alarmMgr.dbErrorsGet(&deActive, &deRaised, &deReleased);
    alarmMgr.badInputGet(&biActive, &biRaised, &biReleased);
    alarmMgr.notificationErrorGet(&neActive, &neRaised, &neReleased);

    alarmMgr.semGive();

    int64_t deRaisedNew   = deRaised   - deRaisedInLastSummary;
    int64_t deReleasedNew = deReleased - deReleasedInLastSummary;
    int64_t neRaisedNew   = neRaised   - neRaisedInLastSummary;
    int64_t neReleasedNew = neReleased - neReleasedInLastSummary;
    int64_t biRaisedNew   = biRaised   - biRaisedInLastSummary;
    int64_t biReleasedNew = biReleased - biReleasedInLastSummary;

    // Round the corner?
    if (deRaisedNew   < 0)  { deRaisedNew   = LONG_MAX - deRaisedInLastSummary   + deRaised;   }
    if (deReleasedNew < 0)  { deReleasedNew = LONG_MAX - deReleasedInLastSummary + deReleased; }
    if (neRaisedNew   < 0)  { neRaisedNew   = LONG_MAX - neRaisedInLastSummary   + neRaised;   }
    if (neReleasedNew < 0)  { neReleasedNew = LONG_MAX - neReleasedInLastSummary + neReleased; }
    if (biRaisedNew   < 0)  { biRaisedNew   = LONG_MAX - biRaisedInLastSummary   + biRaised;   }
    if (biReleasedNew < 0)  { biReleasedNew = LONG_MAX - biReleasedInLastSummary + biReleased; }


    LM_S(("Transactions: %lu (new: %lu)", transactionsNow, diff));

    LM_S(("DB status: %s, raised: (total: %d, new: %d), released: (total: %d, new: %d)",
          deActive? "erroneous" : "ok",
          deRaised,
          deRaisedNew,
          deReleased,
          deReleasedNew));

    LM_S(("Notification failure active alarms: %d, raised: (total: %d, new: %d), released: (total: %d, new: %d)",
          neActive,
          neRaised,
          neRaisedNew,
          neReleased,
          neReleasedNew));

    LM_S(("Bad input active alarms: %d, raised: (total: %d, new: %d), released: (total: %d, new: %d)",
          biActive,
          biRaised,
          biRaisedNew,
          biReleased,
          biReleasedNew));

    deRaisedInLastSummary   = deRaised;
    deReleasedInLastSummary = deReleased;
    neRaisedInLastSummary   = neRaised;
    neReleasedInLastSummary = neReleased;
    biRaisedInLastSummary   = biRaised;
    biReleasedInLastSummary = biReleased;
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
