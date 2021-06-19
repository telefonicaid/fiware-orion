/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <strings.h>                                             // bzero
#include <string.h>                                              // memcpy
#include <semaphore.h>                                           // sem_wait, sem_post

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // kalloc, orionldState
#include "orionld/rest/OrionLdRestService.h"                     // OrionLdRestService
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"    // orionldPostSubscriptions
#include "orionld/serviceRoutines/orionldPostRegistrations.h"    // orionldPostRegistrations
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/contextCache/orionldContextCache.h"            // Context Cache Internals
#include "orionld/contextCache/orionldContextCacheInsert.h"      // Own interface



// -----------------------------------------------------------------------------
//
// originName -
//
const char* originName(OrionldContextOrigin origin)
{
  switch (origin)
  {
  case OrionldContextUnknownOrigin:        return "Unknown";
  case OrionldContextFromInline:           return "Inline";
  case OrionldContextDownloaded:           return "Downloaded";
  case OrionldContextFileCached:           return "FileCached";
  case OrionldContextForNotifications:     return "Notifications";
  case OrionldContextForForwarding:        return "Forwarding";
  case OrionldContextUserCreated:          return "UserCreated";
  }

  return "InvalidOrigin";
}



// -----------------------------------------------------------------------------
//
// orionldContextCacheInsert -
//
void orionldContextCacheInsert(OrionldContext* contextP)
{
  int slotNo = orionldContextCacheSlotIx;

  if (contextP == NULL)
  {
    LM_W(("contextP == NULL"));
    return;
  }

  if (contextP->origin == OrionldContextFromInline)
  {
    //
    // If the request is a subscription creation, then the context must be saved
    // Same same for registrations
    // FIXME: For now it must be saved. In the end, it should be saved just as it came in, in the subscription
    //
    if ((orionldState.serviceP->serviceRoutine != orionldPostSubscriptions) && (orionldState.serviceP->serviceRoutine != orionldPostRegistrations)) 
      return;
  }

  sem_wait(&orionldContextCacheSem);

  if (slotNo >= orionldContextCacheSlots)
  {
    // Look for holes - a context may have been deleted !!!
    for (int ix = 0; ix < orionldContextCacheSlots; ix++)
    {
      if (orionldContextCache[ix] == NULL)
      {
        slotNo = ix;
        break;
      }
    }
  }
  else
    ++orionldContextCacheSlotIx;

  //
  // Reallocation necessary?
  //
  if (slotNo >= orionldContextCacheSlots)
  {
    //
    // NOTE
    //   the code is full of pointers to contexts, so, the contexts themselves CANNOT MOVE.
    //   They don't, as 'orionldContextCache' is an array of OrionldContext*.
    //   The cache itself may be reallocated and move, but what it points to - the contextx - they stay where thry were.
    //
    int   slotsToAdd   = 50;
    int   addedSize    = slotsToAdd * sizeof(OrionldContext*);
    int   newNoOfSlots = orionldContextCacheSlots + slotsToAdd;
    char* newArray     = (char*) kaAlloc(&kalloc, sizeof(OrionldContext*) * newNoOfSlots);

    memcpy(newArray, (char*) orionldContextCache, sizeof(OrionldContext*) * orionldContextCacheSlots);
    bzero(&newArray[sizeof(OrionldContext*) * orionldContextCacheSlots], addedSize);

    orionldContextCacheSlots += 50;
    orionldContextCache = (OrionldContext**) newArray;

    slotNo = orionldContextCacheSlotIx;
  }

  orionldContextCache[slotNo] = contextP;
  sem_post(&orionldContextCacheSem);
}
