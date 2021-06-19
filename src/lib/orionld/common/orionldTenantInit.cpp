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

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant, tenantList, tenantCache
#include "orionld/common/orionldState.h"                         // dbName (CLI param - default is "orion")
#include "orionld/common/tenantList.h"                           // tenantList, tenantSem, tenant0, tenantCache



// -----------------------------------------------------------------------------
//
// orionldTenantInit
//
void orionldTenantInit(void)
{
  if (sem_init(&tenantSem, 0, 1) == -1)
    LM_X(1, ("Runtime Error (error initializing semaphore for orionld tenants: %s)", strerror(errno)));

  //
  // Initialize all path values for the default tenant
  //
  //  tenant0.tenant = "orion" - or whatever the dbName is set to ("orion" is the default value of the database prefix)
  //
  // NOTE: 'dbName' is a global variable from orionld.cpp, and it holds the prefix for the DB, which is configurable with default value set to "orion"
  //       Orion-LD, for backwards compatibility, uses the exact same naming conventions for the databases as Orion.
  //       [ Also, the exact same database layout - inherited from NGSIv1 (OMA NGSI) ]
  //
  tenant0.tenant[0]   = 0;       // The default database has no tenant, so, empty

  snprintf(tenant0.mongoDbName,     sizeof(tenant0.mongoDbName),   "%s",               dbName);
  snprintf(tenant0.entities,        sizeof(tenant0.entities),      "%s.entities",      dbName);
  snprintf(tenant0.subscriptions,   sizeof(tenant0.subscriptions), "%s.csubs",         dbName);
  snprintf(tenant0.avSubscriptions, sizeof(tenant0.subscriptions), "%s.casubs",        dbName);
  snprintf(tenant0.registrations,   sizeof(tenant0.registrations), "%s.registrations", dbName);
  snprintf(tenant0.troeDbName,      sizeof(tenant0.troeDbName),    "%s",               dbName);

  tenantList  = NULL;
  tenantCache = NULL;
}
