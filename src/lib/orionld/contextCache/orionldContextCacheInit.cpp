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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // dbHost, coreContextUrl, builtinCoreContext
#include "orionld/mongoc/mongocContextCacheGet.h"                // mongocContextCacheGet
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextFromBuffer.h"            // orionldContextFromBuffer
#include "orionld/contextCache/orionldContextCache.h"            // orionldContextCacheArray, orionldContextCacheSem
#include "orionld/contextCache/orionldContextCachePersist.h"     // orionldContextCachePersist
#include "orionld/contextCache/orionldContextCacheInit.h"        // Own interface



// -----------------------------------------------------------------------------
//
// dbContextToCache -
//
void dbContextToCache(KjNode* dbContextP, KjNode* atContextP, bool keyValues, bool coreContext)
{
  KjNode* idNodeP        = kjLookup(dbContextP, "_id");
  KjNode* urlNodeP       = kjLookup(dbContextP, "url");
  KjNode* parentNodeP    = kjLookup(dbContextP, "parent");
  KjNode* originNodeP    = kjLookup(dbContextP, "origin");
  KjNode* createdAtNodeP = kjLookup(dbContextP, "createdAt");

  if (idNodeP        == NULL) LM_RVE(("Database Error (No 'id' node in cached context in DB)"));
  if (urlNodeP       == NULL) LM_RVE(("Database Error (No 'url' node in cached context in DB)"));
  if (originNodeP    == NULL) LM_RVE(("Database Error (No 'origin' node in cached context in DB)"));
  if (createdAtNodeP == NULL) LM_RVE(("Database Error (No 'createdAt' node in cached context in DB)"));

  char*                 id           = idNodeP->value.s;
  char*                 url          = urlNodeP->value.s;
  double                createdAt    = createdAtNodeP->value.f;
  OrionldContextOrigin  origin       = orionldOriginFromString(originNodeP->value.s);
  OrionldContext*       contextP     = orionldContextFromTree(url, origin, id, atContextP);

  if (contextP == NULL)
    LM_RVE(("Internal Error (unable to create context '%s' from DB - %s: %s)", url, orionldState.pd.title, orionldState.pd.detail));

  contextP->createdAt   = createdAt;
  contextP->usedAt      = 0;

  if (coreContext == true)
  {
    contextP->coreContext = true;
    orionldCoreContextP   = contextP;
  }

  if (parentNodeP != NULL)
    contextP->parent = parentNodeP->value.s;
}



// -----------------------------------------------------------------------------
//
// orionldContextCacheInit -
//
void orionldContextCacheInit(void)
{
  bzero(&orionldContextCacheArray, sizeof(orionldContextCacheArray));

  if (sem_init(&orionldContextCacheSem, 0, 1) == -1)
    LM_X(1, ("Runtime Error (error initializing semaphore for orionld context list; %s)", strerror(errno)));

  //
  // Retrieve the context cache from the database and populate the context cache in RAM
  //
  KjNode* contextArray = mongocContextCacheGet();

  //
  // Three loops:
  // 1. Core Context + cleanup
  //    - Remove all incorrect contexts from the list
  //    - If the URL of a context is the CORE-CONTEXT-URL - insert in cache and set the global pointer orionldCoreContextP
  //    - Skip all other contexts
  //    - If the core context wasn't found, then download it, create it, insert in cache, persist in DB and set the global pointer orionldCoreContextP
  //
  // 2. Key-Value contexts
  //    - Go over the context list from DB and treat only those whose context is an Object - key-values
  //    - Why?   Well, they have no dependencies, no other contexts may need downloading during this step
  //
  // 3. Array contexts
  //    - Lastly, go over all contexts of Array type
  //

  // 1. Find the Core Context
  if (contextArray != NULL)
  {
    KjNode* contextNodeP = contextArray->value.firstChildP;
    KjNode* next;
    while (contextNodeP != NULL)
    {
      next = contextNodeP->next;

      KjNode* valueNodeP = kjLookup(contextNodeP, "value");
      KjNode* urlNodeP   = kjLookup(contextNodeP, "url");

      if (valueNodeP == NULL)
      {
        LM_E(("Database Error (invalid context in orionld::contexts collection - 'value' node is missing)"));
        kjChildRemove(contextArray, contextNodeP);
      }
      else if (urlNodeP == NULL)
      {
        LM_E(("Database Error (invalid context in orionld::contexts collection - 'url' node is missing)"));
        kjChildRemove(contextArray, contextNodeP);
      }
      else if (strcmp(urlNodeP->value.s, coreContextUrl) == 0)
      {
        kjChildRemove(contextArray, contextNodeP);
        dbContextToCache(contextNodeP, valueNodeP, true, true);
      }

      contextNodeP = next;
    }
  }

  // Still no core context? - download it
  if (orionldCoreContextP == NULL)
  {
    orionldCoreContextP = orionldContextFromUrl(coreContextUrl, NULL);
    if (orionldCoreContextP == NULL)
      LM_W(("Unable to download the core context (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
  }

  // Still no core context? - use the default core context, meant for airgapped setups
  if (orionldCoreContextP == NULL)
  {
    LM_T(LmtContextCache, ("No network?  Getting the core context from builtin"));

    //
    // The builtin core context is a string in a read-only segment.
    // The parser needs to modify it, so, the buffer needs to be copied to read/write memory.
    //
    int   bufLen = strlen(builtinCoreContext);
    char* buf    = (char*) calloc(1, bufLen + 1);

    if (buf == NULL)
      LM_X(1, ("Out of memory trying to allocate %d bytes for the built-in Core Context"));

    memcpy(buf, builtinCoreContext, bufLen + 1);
    orionldCoreContextP = orionldContextFromBuffer(coreContextUrl, OrionldContextBuiltinCoreContext, coreContextUrl, buf);
    free(buf);
    LM_T(LmtContextCache, ("Core Context at %p", orionldCoreContextP));
    if (orionldCoreContextP == NULL)
      LM_X(1, ("Unable to create the core context from in-compiled default core context (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
  }
  LM_T(LmtContextCache, ("Core Context at %p", orionldCoreContextP));

  if (contextArray == NULL)
    return;

  // 2. Key-Value contexts
  KjNode* contextNodeP = contextArray->value.firstChildP;
  KjNode* next;
  while (contextNodeP != NULL)
  {
    next = contextNodeP->next;

    KjNode* valueNodeP = kjLookup(contextNodeP, "value");  // If "value" not present - error in first loop

    // In this second loop, we only deal with @contexts that are Objects - key-value lists
    if (valueNodeP->type == KjObject)
    {
      kjChildRemove(contextArray, contextNodeP);
      dbContextToCache(contextNodeP, valueNodeP, true, false);
    }

    contextNodeP = next;
  }

  // Now all other contexts
  for (contextNodeP = contextArray->value.firstChildP; contextNodeP != NULL; contextNodeP = contextNodeP->next)
  {
    KjNode* valueNodeP = kjLookup(contextNodeP, "value");   // If "value" not present - error in first loop

    if (valueNodeP->type == KjArray)
      dbContextToCache(contextNodeP, valueNodeP, false, false);
    else
      LM_E(("Database Error (invalid context in orionld::contexts collection - not an arry nor an object: %s", kjValueType(valueNodeP->type)));
  }
}
