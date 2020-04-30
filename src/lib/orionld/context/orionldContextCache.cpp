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
#include <unistd.h>                                              // NULL

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextCache.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// Context Cache Internals
//
sem_t             orionldContextCacheSem;
OrionldContext*   orionldContextCacheArray[100];  // When 100 is not enough, a realloc is done (automatically)
OrionldContext**  orionldContextCache         = orionldContextCacheArray;
int               orionldContextCacheSlots    = 100;
int               orionldContextCacheSlotIx   = 0;
