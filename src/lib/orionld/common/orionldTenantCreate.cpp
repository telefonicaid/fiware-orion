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

#include "orionld/common/orionldState.h"                       // tenantV, tenants
#include "orionld/temporal/temporalCommon.h"                   // temporalTenantInitialise
#include "orionld/common/orionldTenantCreate.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldTenantCreate
//
void orionldTenantCreate(char* tenant)
{
  if (tenants >= K_VEC_SIZE(tenantV))
    LM_X(1, ("Too many tenants in the system - increase the size of tenantV and recompile!"));

  tenantV[tenants++] = strdup(tenant);

  temporalTenantInitialise(tenant);
}
