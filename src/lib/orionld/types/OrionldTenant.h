#ifndef SRC_LIB_ORIONLD_TYPES_ORIONLDTENANT_H_
#define SRC_LIB_ORIONLD_TYPES_ORIONLDTENANT_H_

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



// -----------------------------------------------------------------------------
//
// OrionldTenant - the info needed for a tenant
//
typedef struct OrionldTenant
{
  char                   tenant[52];           // Empty if no tenant is used
  char                   mongoDbName[66];      // dbPrefix + "-" + tenant.                     E.g. "orion-openiot"
  char                   entities[88];         // mongo entities collection path.              E.g. "orion-openiot.entities"
  char                   subscriptions[88];    // mongo subscriptions collection path.         E.g. "orion-openiot.csubs"
  char                   avSubscriptions[88];  // mongo reg subscriptions collection path.     E.g. "orion-openiot.casubs"
  char                   registrations[88];    // mongo registrations collection path.         E.g. "orion-openiot.registrations"
  char                   troeDbName[72];       // TRoE database name                           E.g. "orion_openiot"
  struct OrionldTenant*  next;                 // Pointer to the next one in the linked list
} OrionldTenant;

#endif  // SRC_LIB_ORIONLD_TYPES_ORIONLDTENANT_H_
