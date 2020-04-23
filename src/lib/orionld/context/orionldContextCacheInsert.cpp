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
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextCache.h"                 // Context Cache Internals
#include "orionld/context/orionldContextCacheInsert.h"           // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextCacheInsert -
//
void orionldContextCacheInsert(OrionldContext* contextP)
{
  if (contextP == NULL)
  {
    LM_W(("contextP == NULL !!!     FIX IT !!!"));
    return;
  }

  sem_wait(&orionldContextCacheSem);

  //
  // Reallocation necessary?
  //

  if (orionldContextCacheSlotIx >= orionldContextCacheSlots)
  {
    int   slotsToAdd   = 50;
    int   addedSize    = slotsToAdd * sizeof(OrionldContext*);
    int   newNoOfSlots = orionldContextCacheSlots + slotsToAdd;
    char* newArray     = (char*) kaAlloc(&kalloc, sizeof(OrionldContext*) * newNoOfSlots);

    memcpy(newArray, (char*) orionldContextCache, sizeof(OrionldContext*) * orionldContextCacheSlots);
    bzero(&newArray[sizeof(OrionldContext*) * orionldContextCacheSlots], addedSize);

    orionldContextCacheSlots += 50;
    orionldContextCache = (OrionldContext**) newArray;
  }

  orionldContextCache[orionldContextCacheSlotIx] = contextP;
  ++orionldContextCacheSlotIx;

  sem_post(&orionldContextCacheSem);
}
