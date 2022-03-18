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

extern "C"
{
#include "kbase/kMacros.h"                                     // K_VEC_SIZE
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/db/dbConfiguration.h"                        // dbIdIndexCreate
#include "orionld/troe/pgDatabasePrepare.h"                    // pgDatabasePrepare
#include "orionld/types/OrionldTenant.h"                       // OrionldTenant
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/tenantList.h"                         // tenantList
#include "orionld/common/orionldTenantCreate.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldTenantCreate
//
// This function, except for the init phase, is running with the tenant semaphore taken
// (when called from orionldTenantGet)
//
OrionldTenant* orionldTenantCreate(const char* tenantName)
{
  if ((tenantName == NULL) || (tenantName[0] == 0))
  {
    LM_W(("TENANT: Attempt to create the default tenant! (tenantName at %p)", tenantName));
    return &tenant0;
  }

  OrionldTenant* tenantP = (OrionldTenant*) malloc(sizeof(OrionldTenant));

  if (tenantP == NULL)
    LM_RE(NULL, ("Out of memory"));

  snprintf(tenantP->tenant,          sizeof(tenantP->tenant) - 1,          "%s",                  tenantName);
  snprintf(tenantP->mongoDbName,     sizeof(tenantP->mongoDbName) - 1,     "%s-%s",               dbName, tenantName);
  snprintf(tenantP->entities,        sizeof(tenantP->entities) - 1,        "%s-%s.entities",      dbName, tenantName);
  snprintf(tenantP->subscriptions,   sizeof(tenantP->subscriptions) - 1,   "%s-%s.csubs",         dbName, tenantName);
  snprintf(tenantP->avSubscriptions, sizeof(tenantP->avSubscriptions) - 1, "%s-%s.casubs",        dbName, tenantName);
  snprintf(tenantP->registrations,   sizeof(tenantP->registrations) - 1,   "%s-%s.registrations", dbName, tenantName);
  snprintf(tenantP->troeDbName,      sizeof(tenantP->troeDbName) - 1,      "%s_%s",               dbName, tenantName);

  // Add new tenant to tenant list
  tenantP->next = tenantList;  // It's OK if the tenant list is empty (tenantList == NULL)
  tenantList    = tenantP;

  if (idIndex == true)
    dbIdIndexCreate(tenantP);

  // if TRoE is on, need to create the DB in postgres
  if (troe)
  {
    if (pgDatabasePrepare(tenantP->troeDbName) != true)
      LM_E(("Database Error (unable to prepare a new TRoE database for tenant '%s')", orionldState.tenantP->troeDbName));
  }

  return tenantP;
}
