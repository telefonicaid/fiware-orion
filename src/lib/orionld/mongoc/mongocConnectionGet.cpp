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
#include <unistd.h>                                              // NULL
#include <semaphore.h>                                           // sem_wait, sem_post
#include <mongoc/mongoc.h>                                       // mongoc driver

#include "orionld/common/orionldState.h"                         // orionldState, mongocPool
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/mongoc/mongocConnectionGet.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// mongocConnectionGet -
//
void mongocConnectionGet(OrionldTenant* tenantP, DbCollection dbCollection)
{
  sem_wait(&mongocConnectionSem);

  if (orionldState.mongoc.client == NULL)
    orionldState.mongoc.client = mongoc_client_pool_pop(mongocPool);

  if ((dbCollection & DbEntities) == DbEntities)
  {
    if (orionldState.mongoc.entitiesP == NULL)
      orionldState.mongoc.entitiesP = mongoc_client_get_collection(orionldState.mongoc.client, tenantP->mongoDbName, "entities");
  }

  if ((dbCollection & DbSubscriptions) == DbSubscriptions)
  {
    if (orionldState.mongoc.subscriptionsP == NULL)
      orionldState.mongoc.subscriptionsP = mongoc_client_get_collection(orionldState.mongoc.client, tenantP->mongoDbName, "csubs");
  }

  if ((dbCollection & DbRegistrations) == DbRegistrations)
  {
    if (orionldState.mongoc.registrationsP == NULL)
      orionldState.mongoc.registrationsP = mongoc_client_get_collection(orionldState.mongoc.client, tenantP->mongoDbName, "registrations");
  }

  if ((dbCollection & DbContexts) == DbContexts)
  {
    if (orionldState.mongoc.contextsP == NULL)
      orionldState.mongoc.contextsP = mongoc_client_get_collection(orionldState.mongoc.client, "orionld", "contexts");
  }

  sem_post(&mongocConnectionSem);
}
