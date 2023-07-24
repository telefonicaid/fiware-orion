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
#include <pthread.h>                                        // pthread_exit

extern "C"
{
#include "kjson/KjNode.h"                                   // KjNode
}

#include "logMsg/logMsg.h"                                  // LM_x

#include "orionld/common/orionldState.h"                    // orionldState, pernotSubCache
#include "orionld/pernot/PernotSubscription.h"              // PernotSubscription
#include "orionld/pernot/pernotTreat.h"                     // Own interface


extern double currentTime(void);  // FIXME: Move to orionld/common
// -----------------------------------------------------------------------------
//
// pernotTreat -
//
static void* pernotTreat(void* vP)
{
  PernotSubscription* subP = (PernotSubscription*) vP;
  bool                ok   = true;
  double              now  = currentTime();

  // Build the query
  LM_T(LmtPernot, ("Creating the query for pernot-subscription %s", subP->subscriptionId));

  //
  // Timestamps and Status
  //
  if (ok == true)
  {
    subP->lastSuccessTime   = now;
    subP->consecutiveErrors = 0;
  }
  else
  {
    subP->lastFailureTime    = now;
    subP->notificationErrors += 1;
    subP->consecutiveErrors  += 1;

    if (subP->consecutiveErrors >= 3)
    {
      subP->state = SubErroneous;
      LM_W(("%s: 3 consecutive errors - setting the subscription in Error state", subP->subscriptionId));
    }
  }

  subP->notificationAttempts += 1;

  pthread_exit(0);
  return NULL;
}



// -----------------------------------------------------------------------------
//
// pernotTreatStart -
//
void pernotTreatStart(PernotSubscription* subP)
{
  pthread_t tid;

  LM_T(LmtPernot, ("Starting thread for one Periodic Notification Subscription (%s)", subP->subscriptionId));
  pthread_create(&tid, NULL, pernotTreat, subP);

  // It's OK to lose the thread ID ... I think ...
}
