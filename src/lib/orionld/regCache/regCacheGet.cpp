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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/regCache/RegCache.h"                           // RegCache, RegCacheItem, regCacheList
#include "orionld/regCache/regCacheGet.h"                        // Own interface


// -----------------------------------------------------------------------------
//
// regCacheGet - creates a new reg-cache if not found (adds it to reg-cache-list and returns a pointer to the new reg-cache)
//
RegCache* regCacheGet(OrionldTenant* tenantP, bool createIfNotFound)
{
  RegCache* rcP = regCacheList;

  while (rcP != NULL)
  {
    if (strcmp(rcP->tenant, tenantP->tenant) == 0)
      return rcP;
    rcP = rcP->next;
  }

  if (createIfNotFound == false)
    return NULL;

  rcP = (RegCache*) malloc(sizeof(RegCache));
  rcP->tenant  = tenantP->tenant;
  rcP->regList = NULL;
  rcP->next    = regCacheList;

  regCacheList = rcP;

  return rcP;
}
