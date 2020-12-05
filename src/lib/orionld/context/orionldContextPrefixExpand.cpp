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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextPrefixExpand.h"          // Own interface



// -----------------------------------------------------------------------------
//
// prefixCacheLookup -
//
static const char* prefixCacheLookup(const char* str)
{
  for (int ix = 0; ix < orionldState.prefixCache.items; ix++)
  {
    if (strcmp(orionldState.prefixCache.cache[ix].prefix, str) == 0)
      return orionldState.prefixCache.cache[ix].expanded;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// prefixCacheInsert -
//
// If the cache is full, then we reuse the oldest
//
static void prefixCacheInsert(const char* prefix, const char* expansion)
{
  int                     index = orionldState.prefixCache.index % ORIONLD_PREFIX_CACHE_SIZE;
  OrionldPrefixCacheItem* itemP = &orionldState.prefixCache.cache[index];

  itemP->prefix   = (char*) prefix;
  itemP->expanded = (char*) expansion;

  ++orionldState.prefixCache.index;

  if (orionldState.prefixCache.items < ORIONLD_PREFIX_CACHE_SIZE)
    ++orionldState.prefixCache.items;
}



// -----------------------------------------------------------------------------
//
// orionldContextPrefixExpand -
//
// This function looks for a ':' inside 'name' and if found, treats what's before the ':' as a prefix.
// This prefix is looked up in the context and if found, the name is expanded, replacing the prefix (and the colon)
// with the value of the context item found in the lookup.
//
// NOTE
//   * URIs contain ':' but we don't want to expand 'urn', not' http', etc.
//     So, if 'name' starts with 'urn:', or if "://" is found in 'name, then no prefix expansion is performed.
//
//   * Normally, just a few prefixes are used, so a "prefix cache" of 10 values is maintained.
//     This cache is local to the thread, so no semaphores are needed
//
char* orionldContextPrefixExpand(OrionldContext* contextP, const char* str, char* colonP)
{
  char* prefix;
  char* rest;
  char* prefixExpansion;

  // Never expand URNs
  if (SCOMPARE4(str, 'u', 'r', 'n', ':'))
    return (char*) str;

  // Never expand anything xxx://
  if (colonP != NULL)
  {
    if ((colonP[1] == '/') && (colonP[2] == '/'))  // takes care of http:// and https:// and any other "xxx://"
      return (char*) str;
  }

  //
  // "Valid" colon found - need to replace a prefix
  //
  // At this point, 'colonP' points to the ':'
  // The simple parse of 'str' is done, now extract the two parts: 'prefix' and 'rest'
  //
  *colonP = 0;
  prefix  = (char*) str;
  rest    = &colonP[1];

  // Is the prefix in the cache?
  prefixExpansion = (char*) prefixCacheLookup(str);

  // If not, look it up in the context and add it to the cache
  if (prefixExpansion == NULL)
  {
    prefixExpansion = (char*) orionldContextItemExpand(contextP, prefix, false, NULL);
    if (prefixExpansion != NULL)
      prefixCacheInsert(prefix, prefixExpansion);
    else
    {
      //
      // Prefix not found anywhere
      // Fix the broken 'str' (the colon has been nulled out) and return it
      //
      *colonP = ':';
      return (char*) str;
    }
  }

  // Compose the new string
  int    expandedStringLen = strlen(prefixExpansion) + strlen(rest) + 1;
  char*  expandedString    = (char*) kaAlloc(&orionldState.kalloc, expandedStringLen);

  snprintf(expandedString, expandedStringLen, "%s%s", prefixExpansion, rest);


  //
  // Before returning - fix the broken 'str' (the colon has been nulled out)
  //
  *colonP = ':';

  return expandedString;
}

