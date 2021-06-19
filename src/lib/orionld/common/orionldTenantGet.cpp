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
#include <semaphore.h>                                           // sem_init

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/common/tenantList.h"                           // tenantList, tenantSem
#include "orionld/common/orionldTenantLookup.h"                  // orionldTenantLookup
#include "orionld/common/orionldTenantCreate.h"                  // orionldTenantCreate
#include "orionld/common/orionldTenantGet.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// orionldTenantGet
//
OrionldTenant* orionldTenantGet(const char* tenantName)
{
  LM_TMP(("Does the tenant '%s' already exist?", tenantName));
  OrionldTenant* tenantP = orionldTenantLookup(tenantName);
  LM_TMP(("tenantP == %p", tenantP));
  if (tenantP != NULL)
  {
    tenantCache = tenantP;  // No need to semaphore-protect tenantCache
    return tenantP;
  }

  LM_TMP(("tenant '%s' must be created", tenantName));

  //
  // The tenant doesn't exist - it needs to be created:
  //   - Take the tenantSem
  //   - Look up the tenant again
  //     - If it exists now, then somebody created it while we were waiting on the semaphore
  //     - That's unlikely but quite possible, all good
  //   - If the tenant does not exist, create it
  //   - Post the tenantSem
  //   - return the newly created tenant (either it was created here or elsewhere!)
  //
  LM_TMP(("Waiting on tenantSem"));
  sem_wait(&tenantSem);
  LM_TMP(("Looking up the tenant again - this time with semaphore taken - does it still n ot exist?"));
  tenantP = orionldTenantLookup(tenantName);
  LM_TMP(("tenantP == %p", tenantP));

  if (tenantP == NULL)  // Second lookup - this time sem-protected
  {
    LM_TMP(("Tenant '%s' not found - creating it", tenantName));
    tenantP = orionldTenantCreate(tenantName);
  }
  else
    LM_TMP(("Tenant '%s' found in the second try", tenantName));

  tenantCache = tenantP;  // No real need to semaphore-protect tenantCache, but hey, it comes practically for free! :)

  LM_TMP(("Posting tenantSem"));
  sem_post(&tenantSem);

  return tenantP;
}
