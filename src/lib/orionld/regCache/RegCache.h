#ifndef SRC_LIB_ORIONLD_REGCACHE_REGCACHE_H_
#define SRC_LIB_ORIONLD_REGCACHE_REGCACHE_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <regex.h>                                               // regex_t

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "orionld/types/RegistrationMode.h"                      // RegistrationMode
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/context/OrionldContext.h"                      // OrionldContext



// -----------------------------------------------------------------------------
//
// RegDeltas -
//
typedef struct RegDeltas
{
  uint32_t timesSent;
  uint32_t timesFailed;
  double   lastSuccess;
  double   lastFailure;
} RegDeltas;



// -----------------------------------------------------------------------------
//
// RegIdPattern -
//
typedef struct RegIdPattern
{
  regex_t               regex;
  KjNode*               owner;  // Reference to the 'idPattern' in the rciP->regTree that the regex belongs to
  struct RegIdPattern*  next;
} RegIdPattern;



// -----------------------------------------------------------------------------
//
// RegCacheItem -
//
typedef struct RegCacheItem
{
  KjNode*               regTree;
  char*                 regId;         // FIXME: Set when creating registration - points inside regTree, used for debugging only
  RegDeltas             deltas;

  // "Shortcuts" and transformed info, all copies from the regTree - for improved performance
  RegistrationMode      mode;
  uint64_t              opMask;
  OrionldContext*       contextP;      // FIXME: Set when creating/patching registration
  bool                  acceptJsonld;  // Accept is set to application/ld+json
  char*                 ipAndPort;     // IP:port - for X-Forwarded-For
  RegIdPattern*         idPatternRegexList;
  struct RegCacheItem*  next;
} RegCacheItem;



// -----------------------------------------------------------------------------
//
// RegCache -
//
typedef struct RegCache
{
  OrionldTenant* tenantP;
  RegCacheItem*  regList;
  RegCacheItem*  last;
} RegCache;



// -----------------------------------------------------------------------------
//
// RegCacheIterFunc -
//
typedef int (*RegCacheIterFunc)(RegCache* rcP, KjNode* dbRegP);



// -----------------------------------------------------------------------------
//
// regCacheList - head of the list of Registration Caches (one reg-cache per tenant)
//
extern RegCache* regCacheList;

#endif  // SRC_LIB_ORIONLD_REGCACHE_REGCACHE_H_
