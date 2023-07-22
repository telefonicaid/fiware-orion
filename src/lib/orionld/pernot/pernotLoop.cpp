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
static double currentTime(void)
{
  // int gettimeofday(struct timeval *tv, struct timezone *tz);
  struct timeval tv;

  if (gettimeofday(&tv, NULL) != 0)
    LM_RE(0, ("gettimeofday error: %s", strerror(errno)));

  return tv.tv_sec + tv.tv_usec / 1000000;
}



// -----------------------------------------------------------------------------
//
// pernotLoop -
//
static void* pernotLoop(void* vP)
{
  while (1)
  {
    for (PernotSubscription* subP = pernotSubCache.head; subP != NULL; subP = subP->next)
    {
      double now = currentTime();

      if (subP->state == SubPaused)
        continue;

      if (subP->expiresAt <= now)
        subP->state = SubExpired;  // Should it be removed?

      if (subP->state == SubExpired)
        continue;

      if (subP->state == SubErroneous)
      {
        // Check for cooldown - take it out of error if cooldown time has passwd
        if (subP->lastFailureTime + subP->cooldown < now)
          subP->state = SubActive;
        else
          continue;
      }

      if (subP->lastNotificationAttempt + subP->timeInterval < now)
      {
        subP->lastNotificationAttempt = now;  // Either it works or fails, the timestamp needs to be updated
        if (pernotTreat(subP) == true)        // Just send the notification, don't await any response
        {
          subP->lastSuccessTime   = now;
          subP->consecutiveErrors = 0;
        }
        else
        {
          subP->lastFailureTime    = now;
          subP->consecutiveErrors += 1;

          if (subP->consecutiveErrors >= 3)
          {
            subP->state = SubErroneous;
          }
        }
      }
    }
  }
}



pthread_t pernotThreadID;
// -----------------------------------------------------------------------------
//
// pernotLoopStart -
//
void pernotLoopStart(void)
{
  pthread_create(&pernotThreadID, NULL, pernotLoop, NULL);
}
