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
#include <string.h>                                            // strcpy

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/OrionldTenant.h"                       // OrionldTenant
#include "orionld/common/tenantList.h"                         // tenantList, tenant0, tenantCache
#include "orionld/common/orionldTenantLookup.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldTenantLookup
//
OrionldTenant* orionldTenantLookup(const char* tenantName)
{
  if ((tenantName == NULL) || (tenantName[0] == 0))
    return &tenant0;

  //
  // cachedTenantP - if I don't do this, I'd have to semaphore-protect 'tenantCache' -
  //
  // as 'tenantCache' could be modified between the strcmp and the "return tenantCache"
  // Let's avoid semaphores by simply using a help variable !!!
  //
  OrionldTenant* cachedTenantP;

  if ((cachedTenantP = tenantCache) != NULL)
  {
    if (strcmp(tenantName, cachedTenantP->tenant) == 0)
      return cachedTenantP;
  }


  // OK ... we'll have to look it up then :(
  OrionldTenant* tenantP = tenantList;

  while (tenantP != NULL)
  {
    if (strcmp(tenantName, tenantP->tenant) == 0)
      return tenantP;

    tenantP = tenantP->next;
  }

  return NULL;
}
