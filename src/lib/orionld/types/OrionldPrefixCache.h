#ifndef SRC_LIB_ORIONLD_COMMON_ORIONLDPREFIXCACHE_H_
#define SRC_LIB_ORIONLD_COMMON_ORIONLDPREFIXCACHE_H_

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



// -----------------------------------------------------------------------------
//
// Prefix Cache
//
//   When prefixes are used, normally the very same, or very few prefixes are used.
//   So, it makes sense to implement a cache for the prefix lookups.
//   A simple vector of 10 key-values are used for this, as normal use-cases don't use
//   more than just a few prefixes.
//   And, the smaller the cache, the quicker the lookups (especially the no-hits).
//



// -----------------------------------------------------------------------------
//
// ORIONLD_PREFIX_CACHE_SIZE - number of slots in the prefix cache
//
#define ORIONLD_PREFIX_CACHE_SIZE 10



// -----------------------------------------------------------------------------
//
// OrionldPrefixCacheItem -
//
typedef struct OrionldPrefixCacheItem
{
  char* prefix;
  char* expanded;
} OrionldPrefixCacheItem;



// -----------------------------------------------------------------------------
//
// OrionldPrefixCache -
//
typedef struct OrionldPrefixCache
{
  int                     index;
  int                     items;
  OrionldPrefixCacheItem  cache[ORIONLD_PREFIX_CACHE_SIZE];
} OrionldPrefixCache;

#endif  // SRC_LIB_ORIONLD_COMMON_ORIONLDPREFIXCACHE_H_
