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
#include <string.h>                                              // strcmp

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextCachePresent.h"          // orionldContextCachePresent
#include "orionld/context/orionldContextCache.h"                 // Context Cache Internals
#include "orionld/context/orionldContextCacheLookup.h"           // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextCacheLookup -
//
OrionldContext* orionldContextCacheLookup(const char* url)
{
  LM_TMP(("CTX: Looking up context '%s'", url));
  orionldContextCachePresent("CTX", "orionldContextCacheLookup");
  for (int ix = 0; ix < orionldContextCacheSlotIx; ix++)
  {
    if (strcmp(url, orionldContextCache[ix]->url) == 0)
      return orionldContextCache[ix];

    if ((orionldContextCache[ix]->id != NULL) && (strcmp(url, orionldContextCache[ix]->id) == 0))
      return orionldContextCache[ix];
  }

  LM_TMP(("CTX: did not find context '%s'", url));
  return NULL;
}
