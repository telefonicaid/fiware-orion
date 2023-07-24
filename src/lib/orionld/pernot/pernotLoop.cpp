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

#include "orionld/common/orionldState.h"                    // orionldState, pernotSubCache
#include "orionld/pernot/PernotSubscription.h"              // PernotSubscription
#include "orionld/pernot/PernotSubCache.h"                  // PernotSubCache
#include "orionld/pernot/pernotTreat.h"                     // pernotTreat
#include "orionld/pernot/pernotLoop.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// pernotSubInsert - move to pernot/pernotSubInsert.h/cpp
//
bool pernotSubInsert(void)
{
  return true;
}



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
// pernotLoop -
//
static void* pernotLoop(void* vP)
{
  while (1)
  {
    double minDiff;

    minDiff = 1;  // Initialize with 1 second, which is the MAX sleep period

    for (PernotSubscription* subP = pernotSubCache.head; subP != NULL; subP = subP->next)
    {
      double now = currentTime();

      if (subP->state == SubPaused)
      {
        LM_T(LmtPernot, ("%s: Paused", subP->subscriptionId));
        continue;
      }

      if ((subP->expiresAt > 0) && (subP->expiresAt <= now))
        subP->state = SubExpired;  // Should it be removed?

      if (subP->state == SubExpired)
      {
        LM_T(LmtPernot, ("%s: Expired", subP->subscriptionId));
        continue;
      }

      if (subP->state == SubErroneous)
      {
        // Check for cooldown - take it out of error if cooldown time has passwd
        if (subP->lastFailureTime + subP->cooldown < now)
          subP->state = SubActive;
        else
        {
          LM_T(LmtPernot, ("%s: Erroneous", subP->subscriptionId));
          continue;
        }
      }

      double diffTime = subP->lastNotificationAttempt + subP->timeInterval - now;
      // LM_T(LmtPernot, ("%s: lastNotificationAttempt: %f", subP->subscriptionId, subP->lastNotificationAttempt));
      // LM_T(LmtPernot, ("%s: timeInterval:            %f", subP->subscriptionId, subP->timeInterval));
      // LM_T(LmtPernot, ("%s: now:                     %f", subP->subscriptionId, now));
      // LM_T(LmtPernot, ("%s: diffTime:                %f", subP->subscriptionId, diffTime));

      if (diffTime <= 0)
      {
        subP->lastNotificationAttempt = now;  // Either it works or fails, the timestamp needs to be updated
        pernotTreatStart(subP);  // Query runs in a new thread

        // Restart the min diff time
        minDiff = 0;
        break;
      }
      else
        minDiff = K_MIN(minDiff, diffTime);
    }

    if (minDiff != 0)
      usleep(minDiff * 1000000);
  }

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
