#ifndef SRC_LIB_ORIONLD_COMMON_TENANTLIST_H_
#define SRC_LIB_ORIONLD_COMMON_TENANTLIST_H_

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
#include <semaphore.h>                                           // sem_t

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant



// -----------------------------------------------------------------------------
//
// tenantSem - semaphore to protect 'tenantList' - the list of tenants
//
extern sem_t tenantSem;



// -----------------------------------------------------------------------------
//
// tenantList - a list of tenants
//
extern OrionldTenant* tenantList;



// -----------------------------------------------------------------------------
//
// tenantCache - last used tenant, for quicker lookups
//
// Reference to the last looked up tenant - no semaphore protection - best effort
//
extern OrionldTenant* tenantCache;



// -----------------------------------------------------------------------------
//
// tenant0 - the default tenant
//
extern OrionldTenant tenant0;

#endif  // SRC_LIB_ORIONLD_COMMON_TENANTLIST_H_
