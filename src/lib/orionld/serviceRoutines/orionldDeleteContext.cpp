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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
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
    LM_W(("Bad Input (context '%s' not found)", id));
    orionldErrorResponseCreate(OrionldResourceNotFound, "Context Not Found", id);
    orionldState.httpStatusCode = 404;
    return false;
  }


  if (orionldState.uriParams.reload == true)
  {
    if (oldContextP->origin != OrionldContextDownloaded)
    {
      LM_W(("Bad Input (only cached contexts are subject for reload)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Wrong type of context", "only cached contexts are subject for reload");
      orionldState.httpStatusCode = 400;
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
      LM_E(("Context Cache Error (context to be reloaded not found in cache)"));  // orionldContextCacheLookup found it though ... ???
      orionldErrorResponseCreate(OrionldInternalError, "Context Cache Error", "context to be reloaded not found in cache");
      orionldState.httpStatusCode = 500;
      return false;
    }

    //
    // Download, parse and insert the new context
    // If anything goes wrong, the old context is put back into the cache
    //
    OrionldProblemDetails pd;
    OrionldContext*       contextP = orionldContextFromUrl(oldContextP->url, oldContextP->id, &pd);

    if (contextP == NULL)
    {
      LM_W(("orionldContextFromUrl(%s): %s: %s", oldContextP->url, pd.title, pd.detail));
      orionldErrorResponseCreate(pd.type, pd.title, pd.detail);
      orionldState.httpStatusCode = pd.status;

      orionldContextCacheInsert(oldContextP);
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
      orionldErrorResponseCreate(OrionldResourceNotFound, "Context Not Found", id);
      return false;
    }
  }

  orionldState.httpStatusCode = 204;
  return true;
}
