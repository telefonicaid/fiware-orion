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
#include <semaphore.h>                                           // sem_wait, sem_post
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // mongoc driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, mongoContextsSem
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocContextCacheDelete.h"             // Own interface


// -----------------------------------------------------------------------------
//
// mongocContextCacheDelete -
//
void mongocContextCacheDelete(const char* id)
{
  bson_t selector;

  mongocConnectionGet(NULL, DbContexts);

  bson_init(&selector);
  bson_append_utf8(&selector, "_id", 3, id, -1);

  sem_wait(&mongocContextsSem);

  bson_error_t  mcError;
  bool          r = mongoc_collection_delete_one(orionldState.mongoc.contextsP, &selector, NULL, NULL, &mcError);

  sem_post(&mongocContextsSem);

  if (r == false)
    LM_E(("Database Error (deleting context '%s': %s)", id, mcError.message));

  bson_destroy(&selector);
}
