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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/common/orionldState.h"                         // kalloc, orionldState
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextCreate.h"                // orionldContextCreate
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/orionldContextCacheInsert.h"           // orionldContextCacheInsert
#include "orionld/context/orionldContextCache.h"                 // ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE
#include "orionld/context/orionldContextHashTablesFill.h"        // orionldContextHashTablesFill
#include "orionld/context/orionldContextFromObject.h"            // Own interface



// -----------------------------------------------------------------------------
//
// hashCode -
//
unsigned int hashCode(const char* name)
{
  unsigned int code = 0;

  while (*name != 0)
  {
    code += (unsigned char) *name;
    ++name;
  }

  return code;
}



// -----------------------------------------------------------------------------
//
// nameCompareFunction -
//
static int nameCompareFunction(const char* name, void* itemP)
{
  OrionldContextItem* cItemP = (OrionldContextItem*) itemP;

  return strcmp(name, cItemP->name);
}



// ----------------------------------------------------------------------------
//
// valueCompareFunction -
//
static int valueCompareFunction(const char* longname, void* itemP)
{
  OrionldContextItem* cItemP = (OrionldContextItem*) itemP;

  return strcmp(longname, cItemP->id);
}



// -----------------------------------------------------------------------------
//
// orionldContextFromObject -
//
// If the context object 'contextObjectP' is part of an array, then it's a local context and
// it is not served.
// Served contexts need to be cloned so that they can be copied back to the caller (GET /ngsi-ld/ex/contexts/xxx).
// For example, the URL "http:/x.y.z/contexts/context1.jsonld" was downloaded and its content is a key-value object.
//
OrionldContext* orionldContextFromObject(char* url, OrionldContextOrigin origin, bool toBeCloned, KjNode* contextObjectP, OrionldProblemDetails* pdP)
{
  OrionldContext*  contextP;
  char*            id = NULL;
  bool             ok = true;

  if (url == NULL)
    url  = orionldContextUrlGenerate(&id);

  contextP = orionldContextCreate(url, origin, id, contextObjectP, true, toBeCloned);
  if (contextP == NULL)
  {
    LM_E(("orionldContextCreate failed"));
    return NULL;
  }

  contextP->context.hash.nameHashTable  = khashTableCreate(&kalloc, hashCode, nameCompareFunction,  ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE);
  if (contextP->context.hash.nameHashTable == NULL)
  {
    LM_E(("khashTableCreate failed"));
    ok = false;
  }

  contextP->context.hash.valueHashTable = khashTableCreate(&kalloc, hashCode, valueCompareFunction, ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE);
  if (contextP->context.hash.valueHashTable == NULL)
  {
    LM_E(("khashTableCreate failed"));
    ok = false;
  }

  if ((ok == true) && (orionldContextHashTablesFill(contextP, contextObjectP, pdP) == false))
  {
    // orionldContextHashTablesFill fills in pdP
    LM_E(("orionldContextHashTablesFill failed"));
    ok = false;
  }

  if (ok == false)
  {
    if (toBeCloned == true)
      kjFree(contextP->tree);
    return NULL;
  }

  orionldContextCacheInsert(contextP);
  return contextP;
}
