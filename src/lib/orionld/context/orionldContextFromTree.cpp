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
extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/common/orionldState.h"                         // orionldState, kalloc, coreContextUrl
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/orionldContextSimplify.h"              // orionldContextSimplify
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromObject.h"            // orionldContextFromObject
#include "orionld/context/orionldContextCreate.h"                // orionldContextCreate
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/contextCache/orionldContextCacheInsert.h"      // orionldContextCacheInsert
#include "orionld/context/orionldContextFromTree.h"              // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextFromTree -
//
OrionldContext* orionldContextFromTree(char* url, OrionldContextOrigin origin, char* id, KjNode* contextTreeP)
{
  int itemsInArray;

  if (contextTreeP->type == KjArray)
  {
    contextTreeP = orionldContextSimplify(contextTreeP, &itemsInArray);
    if ((contextTreeP == NULL) || (contextTreeP->value.firstChildP == NULL))
    {
      // Nothing left in  the array - only Core Context was there but has been removed?
      orionldState.pd.status = 200;

      return orionldCoreContextP;
    }

    if ((itemsInArray == 1) && (contextTreeP->value.firstChildP->type == KjString))
      orionldState.link = contextTreeP->value.firstChildP->value.s;

    //
    // Need to clone the array and add it to the context cache
    //
    bool            arrayToCache = (url != NULL);
    OrionldContext* contextP     = orionldContextCreate(url, origin, id, contextTreeP, false);

    //
    // Once created, the parameter 'url' needs to be "invalidated", as it's already been used - to avoid to use the same URL for children of the context
    //
    url = NULL;
    id  = NULL;

    if (contextP == NULL)
    {
      LM_E(("Internal Error (unable to create context)"));
      return NULL;
    }


    contextP->context.array.items     = itemsInArray;
    contextP->context.array.vector    = (OrionldContext**) kaAlloc(&kalloc, itemsInArray * sizeof(OrionldContext*));

    int ix = 0;
    for (KjNode* ctxItemP = contextTreeP->value.firstChildP; ctxItemP != NULL; ctxItemP = ctxItemP->next)
    {
      OrionldContext* cachedContextP = NULL;

      if (ctxItemP->type == KjString)
        cachedContextP = orionldContextCacheLookup(ctxItemP->value.s);
      else if ((ctxItemP->type != KjObject) && (ctxItemP->type != KjArray))
      {
        orionldError(OrionldBadRequestData, "Invalid @context - invalid type for @context array item", kjValueType(ctxItemP->type), 400);

        if (contextP->url != NULL)  // Only contexts with a URL are "kj-cloned"
          kjFree(contextP->tree);

        return NULL;
      }

      if (cachedContextP == NULL)
      {
        if (ctxItemP->type == KjString)
        {
          url = ctxItemP->value.s;
          id  = (char*) "downloaded";  // FIXME: perhaps NULL is a better value ...
        }
        else if (origin == OrionldContextUserCreated)
          url = orionldContextUrlGenerate(&id);

        contextP->context.array.vector[ix] = orionldContextFromTree(url, origin, id, ctxItemP);
        if (contextP->context.array.vector[ix] != NULL)
          contextP->context.array.vector[ix]->parent = contextP->id;
        else
          LM_RE(NULL, ("unable to download context '%s'", url));
      }
      else
        contextP->context.array.vector[ix] = cachedContextP;

      // Again, url and id needs to be NULLed out for the next loop
      url = NULL;
      id  = NULL;

      ++ix;
    }

    if (arrayToCache == true)
      orionldContextCacheInsert(contextP);

    return contextP;
  }
  else if (contextTreeP->type == KjString)
  {
    if (url != NULL)
    {
      OrionldContext* contextP = orionldContextCacheLookup(url);

      if (contextP == NULL)
      {
        contextP = orionldContextCreate(url, origin, id, contextTreeP, false);

        contextP->context.array.items     = 1;
        contextP->context.array.vector    = (OrionldContext**) kaAlloc(&kalloc, 1 * sizeof(OrionldContext*));
        contextP->context.array.vector[0] = orionldContextFromUrl(contextTreeP->value.s, NULL);

        if (contextP->context.array.vector[0] == NULL)
          LM_RE(NULL, ("Context Error from orionldContextFromUrl"));
      }

      if (contextP != NULL)
        contextP->origin = origin;

      return contextP;
    }
    else
    {
      OrionldContext* contextP;
      contextP = orionldContextFromUrl(contextTreeP->value.s, NULL);
      if (contextP != NULL)
      {
        if (contextP->origin != OrionldContextDownloaded)
          contextP->origin = origin;
      }

      return contextP;
    }
  }
  else if (contextTreeP->type == KjObject)
  {
    OrionldContext* contextP = orionldContextFromObject(url, origin, id, contextTreeP);

    if (contextP)
      contextP->origin = origin;

    return contextP;
  }

  //
  // None of the above. Error
  //
  orionldError(OrionldBadRequestData, "Invalid type for item in @context array", kjValueType(contextTreeP->type), 400);
  return NULL;
}
