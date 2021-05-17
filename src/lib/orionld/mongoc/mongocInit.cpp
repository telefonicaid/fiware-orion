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
#include <semaphore.h>                                           // sem_init
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState



// -----------------------------------------------------------------------------
//
// mongocInit -
//
void mongocInit(const char* dbHost, const char* dbName)
{
  bson_error_t mongoError;
  char         mongoUri[512];

  snprintf(mongoUri, sizeof(mongoUri), "mongodb://%s", dbHost);

  //
  // Initialize libmongoc's internals
  //
  mongoc_init();

  //
  // Safely create a MongoDB URI object from the given string
  //
  orionldState.mongoUri = mongoc_uri_new_with_error(mongoUri, &mongoError);
  if (orionldState.mongoUri == NULL)
    LM_X(1, ("mongoc_uri_new_with_error(%s): %s", orionldState.mongoUri, mongoError.message));

  //
  // Create a new client instance
  //
  orionldState.mongoClient = mongoc_client_new_from_uri(orionldState.mongoUri);
  if (orionldState.mongoClient == NULL)
    LM_X(1, ("mongoc_client_new_from_uri failed"));

  //
  // Register the application name (to get tracking possibilities in the profile logs on the server)
  //
  mongoc_client_set_appname(orionldState.mongoClient, "orionld");

  //
  // Get a handle on the database
  //
  orionldState.mongoDatabase = mongoc_client_get_database(orionldState.mongoClient, dbName);
  if (orionldState.mongoDatabase == NULL)
    LM_X(1, ("DB: mongoc_client_get_database FAILED"));

  //
  // Get a handle on the collections
  //
#ifdef DB_DRIVER_MONGOC
  mongoEntitiesCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "entities");
  if (mongoEntitiesCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'entities') failed", dbName));

  mongoSubscriptionsCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "subscriptions");
  if (mongoSubscriptionsCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'regiatrations') failed", dbName));

  mongoRegistrationsCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "registrations");
  if (mongoRegistrationsCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'regiatrations') failed", dbName));
#endif

  // The Context Cache module uses mongoc regardless
  mongoContextsCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "contexts");
  if (mongoContextsCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'contexts') failed", dbName));


  //
  // For now, there will be a single connection to the 'contexts' collection on DB 'orionld'.
  // A semaphore is needed
  //
  sem_init(&mongoContextsSem, 0, 1);  // 0: shared between threads of the same process. 1: free to be taken
}
