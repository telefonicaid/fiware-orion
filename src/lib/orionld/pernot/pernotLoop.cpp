/*
*
* Copyright 2023 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <string.h>                                         // strerror
#include <sys/time.h>                                       // gettimeofday
#include <pthread.h>                                        // pthread_create
#include <unistd.h>                                         // usleep

extern "C"
{
#include "kbase/kMacros.h"                                  // K_MIN
}

#include "logMsg/logMsg.h"                                  // LM_x

#include "orionld/types/PernotSubscription.h"               // PernotSubscription
#include "orionld/types/PernotSubCache.h"                   // PernotSubCache
#include "orionld/common/orionldState.h"                    // orionldState, pernotSubCache
#include "orionld/mongoc/mongocSubCountersUpdate.h"         // mongocSubCountersUpdate
#include "orionld/pernot/pernotTreat.h"                     // pernotTreat
#include "orionld/pernot/pernotLoop.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// currentTime -
//
double currentTime(void)
{
  // int gettimeofday(struct timeval *tv, struct timezone *tz);
  struct timeval tv;

  if (gettimeofday(&tv, NULL) != 0)
    LM_RE(0, ("gettimeofday error: %s", strerror(errno)));

  return tv.tv_sec + tv.tv_usec / 1000000.0;
}



// -----------------------------------------------------------------------------
//
// pernotSubCacheFlushToDb -
//
void pernotSubCacheFlushToDb(void)
{
  for (PernotSubscription* subP = pernotSubCache.head; subP != NULL; subP = subP->next)
  {
    if (subP->dirty == false)
      continue;

    LM_T(LmtPernotFlush, ("%s: flushing to db (noMatch=%d, notificationAttempts=%d)", subP->subscriptionId, subP->noMatch, subP->notificationAttempts));
    mongocSubCountersUpdate(subP->tenantP,
                            subP->subscriptionId,
                            true,
                            subP->notificationAttempts,
                            subP->notificationErrors,
                            subP->noMatch,
                            subP->lastNotificationTime,
                            subP->lastSuccessTime,
                            subP->lastFailureTime,
                            false);

    // Reset counters
    subP->dirty                   = 0;
    subP->notificationAttemptsDb += subP->notificationAttempts;
    subP->notificationAttempts    = 0;
    subP->notificationErrorsDb   += subP->notificationErrors;
    subP->notificationErrors      = 0;
    subP->noMatchDb              += subP->noMatch;
    subP->noMatch                 = 0;
  }
}



// -----------------------------------------------------------------------------
//
// pernotSubCacheRefresh -
//
void pernotSubCacheRefresh(void)
{
}



// -----------------------------------------------------------------------------
//
// pernotLoop -
//
static void* pernotLoop(void* vP)
{
  double       nextFlushAt             = currentTime() + subCacheFlushInterval;
  double       nextCacheRefreshAt      = currentTime() + subCacheInterval;
  double       now                     = 0;

  while (1)
  {
    for (PernotSubscription* subP = pernotSubCache.head; subP != NULL; subP = subP->next)
    {
      now = currentTime();

      if (subP->state == SubPaused)
      {
        LM_T(LmtPernotLoop, ("%s: Paused", subP->subscriptionId));
        continue;
      }

      if (subP->isActive == false)
      {
        LM_T(LmtPernotLoop, ("%s: Inactive", subP->subscriptionId));
        continue;
      }

      if ((subP->expiresAt > 0) && (subP->expiresAt <= now))
        subP->state = SubExpired;  // Should it be removed?

      if (subP->state == SubExpired)
      {
        LM_T(LmtPernotLoop, ("%s: Expired", subP->subscriptionId));
        continue;
      }

      if (subP->state == SubErroneous)
      {
        // Check for cooldown - take it out of error if cooldown time has passwd
        if (subP->lastFailureTime + subP->cooldown < now)
          subP->state = SubActive;
        else
        {
          LM_T(LmtPernotLoop, ("%s: Erroneous", subP->subscriptionId));
          continue;
        }
      }

      double diffTime = subP->lastNotificationTime + subP->timeInterval - now;
      LM_T(LmtPernotLoopTimes, ("%s: lastNotificationTime:    %f", subP->subscriptionId, subP->lastNotificationTime));
      LM_T(LmtPernotLoopTimes, ("%s: now:                     %f", subP->subscriptionId, now));
      LM_T(LmtPernotLoopTimes, ("%s: diffTime:                %f", subP->subscriptionId, diffTime));
      LM_T(LmtPernotLoopTimes, ("%s: noMatch:                 %d", subP->subscriptionId, subP->noMatch));
      LM_T(LmtPernotLoopTimes, ("%s: noMatchDb:               %d", subP->subscriptionId, subP->noMatchDb));
      LM_T(LmtPernotLoopTimes, ("%s: notificationAttempts:    %d", subP->subscriptionId, subP->notificationAttempts));
      LM_T(LmtPernotLoopTimes, ("%s: notificationAttemptsDb:  %d", subP->subscriptionId, subP->notificationAttemptsDb));

      if (diffTime <= 0)
      {
        LM_T(LmtPernotLoop, ("%s: ---------- Sending notification at %f", subP->subscriptionId, now));
        subP->lastNotificationTime = now;  // Either it works or fails, the timestamp needs to be updated (it's part of the loop)
        pernotTreatStart(subP);            // Query runs in a new thread, loop continues
      }
    }

    if ((subCacheFlushInterval > 0) && (now > nextFlushAt))
    {
      LM_T(LmtPernotLoop, ("Flushing Pernot SubCache contents to DB"));
      pernotSubCacheFlushToDb();
      nextFlushAt += subCacheFlushInterval;
    }
    else if ((subCacheInterval > 0) && (now > nextCacheRefreshAt))
    {
      LM_T(LmtPernotLoop, ("Refreshing Pernot SubCache contents from DB"));
      pernotSubCacheRefresh();
      nextCacheRefreshAt  += subCacheInterval;
    }
    else
    {
      // LM_T(LmtPernotLoop, ("Sleeping 50ms"));
      usleep(50000);  // We always end up here if there are no subscriptions in the cache
    }
  }
  LM_T(LmtPernotLoop, ("End of loop"));
  return NULL;
}



pthread_t pernotThreadID;
// -----------------------------------------------------------------------------
//
// pernotLoopStart -
//
void pernotLoopStart(void)
{
  LM_T(LmtPernot, ("Starting thread for the Periodic Notification Loop"));
  pthread_create(&pernotThreadID, NULL, pernotLoop, NULL);
}
