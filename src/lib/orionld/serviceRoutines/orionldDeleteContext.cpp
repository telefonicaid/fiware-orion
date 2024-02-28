/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/contextCache/orionldContextCache.h"            // Context Cache Internals
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/contextCache/orionldContextCacheDelete.h"      // orionldContextCacheDelete
#include "orionld/contextCache/orionldContextCacheInsert.h"      // orionldContextCacheInsert
#include "orionld/serviceRoutines/orionldDeleteContext.h"        // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteContext -
//
bool orionldDeleteContext(void)
{
  char*            id          = orionldState.wildcard[0];
  OrionldContext*  oldContextP = orionldContextCacheLookup(id);

  if (oldContextP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Context Not Found", id, 404);
    return false;
  }

  if ((oldContextP == orionldCoreContextP) && (orionldState.uriParams.reload == false))
  {
    orionldError(OrionldBadRequestData, "The NGSI-LD Core Context cannot be deleted", "The Core Context can be reloaded, please use the URI param '?reload=true' for that", 400);
    return false;
  }

  if (orionldState.uriParams.reload == true)
  {
    //
    // Only contexts of type Cached (indirect download) can be reloaded
    //
    if (oldContextP->kind != OrionldContextCached)
    {
      orionldError(OrionldBadRequestData, "Wrong type of context", "only cached contexts are subject for reload", 400);
      return false;
    }

    //
    // Remove old context from cache (and keep a pointer to) the old context
    //
    bool done = false;
    for (int ix = 0; ix < orionldContextCacheSlotIx; ix++)
    {
      if (orionldContextCache[ix] == oldContextP)
      {
        orionldContextCache[ix] = NULL;
        done = true;
        break;
      }
    }

    if (done == false)
    {
      orionldError(OrionldInternalError, "Context Cache Error", "context to be reloaded not found in cache", 500);
      return false;
    }

    //
    // Download, parse and insert the new context
    // If anything goes wrong, the old context is put back into the cache
    //
    OrionldContext* contextP = orionldContextFromUrl(oldContextP->url, oldContextP->id);
    if (contextP == NULL)
    {
      orionldContextCacheInsert(oldContextP);
      // orionldError(OrionldLdContextNotAvailable, "Unable to reload the @context", oldContextP->url, 504);
      return false;
    }

    contextP->createdAt      = oldContextP->createdAt;
    contextP->usedAt         = orionldState.requestTime;

    // Free the kj tree of the now obsolete old context
    kjFree(oldContextP->tree);
  }
  else
  {
    if (orionldContextCacheDelete(id) == false)
    {
      orionldState.httpStatusCode = 404;
      orionldError(OrionldResourceNotFound, "Context Not Found", id, 400);
      return false;
    }
  }

  orionldState.httpStatusCode = 204;
  return true;
}
