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
#include <stdio.h>                                                // snprintf
#include <string.h>                                               // strncpy

extern "C"
{
#include "kalloc/kaAlloc.h"                                       // kaAlloc
#include "kalloc/kaStrdup.h"                                      // kaStrdup
}

#include "logMsg/logMsg.h"                                        // LM_*

#include "orionld/common/orionldState.h"                          // kalloc
#include "orionld/common/dotForEq.h"                              // dotForEq
#include "orionld/mongoc/mongocConnectionGet.h"                   // mongocConnectionGet
#include "orionld/mongoc/mongocIdIndexCreate.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocIdIndexCreate -
//
bool mongocIdIndexCreate(OrionldTenant* tenantP)
{
  char*  collectionName = (char*) "entities";
  bson_t key;

  bson_init(&key);
  BSON_APPEND_INT32(&key, "_id.id", 1);

  mongocConnectionGet(NULL, DbNone);

  mongoc_database_t*  dbP                = mongoc_client_get_database(orionldState.mongoc.client, tenantP->mongoDbName);
  char*               indexName          = mongoc_collection_keys_to_index_string(&key);
  bson_t*             createIndexCommand = BCON_NEW("createIndexes",
                                                    BCON_UTF8(collectionName),
                                                    "indexes",
                                                    "[",
                                                    "{",
                                                    "key",
                                                    BCON_DOCUMENT(&key),
                                                    "name",
                                                    BCON_UTF8(indexName),
                                                    "}",
                                                    "]");

  bson_error_t  mcError;
  bson_t        reply;

  if (mongoc_database_write_command_with_opts(dbP, createIndexCommand, NULL, &reply, &mcError) == false)
  {
    LM_E(("Database Error (error creating index for _id.id for db '%s': %s)", tenantP->mongoDbName, mcError.message));
    return false;
  }

  bson_destroy(&key);
  bson_free(indexName);
  bson_destroy(createIndexCommand);
  mongoc_database_destroy(dbP);
  bson_destroy(&reply);

  return true;
}
