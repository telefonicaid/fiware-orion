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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/common/orionldState.h"                         // orionldState, kalloc



// -----------------------------------------------------------------------------
//
// orionldContextCreate -
//
// FIXME: If the context is to be saved in the cache, then 'kalloc' can't be used.
//        Might need two version of this function, one for kaAlloc, one for malloc
//
int cloned = 0;
OrionldContext* orionldContextCreate(const char* url, OrionldContextOrigin origin, const char* id, KjNode* tree, bool keyValues)
{
  OrionldContext* contextP = (OrionldContext*) kaAlloc(&kalloc, sizeof(OrionldContext));

  if (contextP == NULL)
    LM_X(1, ("out of memory - trying to allocate a OrionldContext of %d bytes", sizeof(OrionldContext)));

  contextP->origin    = origin;
  contextP->kind      = OrionldContextCached;  // Default. Changed later to Hosted/Implicit if needed
  contextP->parent    = NULL;
  contextP->createdAt = orionldState.requestTime;
  contextP->usedAt    = orionldState.requestTime;

  // NULL URL means NOT to be saved - will live just inside the request-thread
  if (url != NULL)
  {
    contextP->url   = kaStrdup(&kalloc, url);
    contextP->id    = (id != NULL)? kaStrdup(&kalloc, id) : NULL;

    //
    // If just a string, no clone needed
    //
    if (tree->type != KjString)
      ++cloned;
    contextP->tree = (tree->type != KjString)? kjClone(NULL, tree) : tree;
  }
  else
  {
    contextP->tree = tree;
    contextP->url  = NULL;
    contextP->id   = NULL;
  }

  contextP->keyValues = keyValues;
  contextP->lookups   = 1;

  return contextP;
}
