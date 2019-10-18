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

  LM_TMP(("DB: initializing database"));
  snprintf(mongoUri, sizeof(mongoUri), "mongodb://%s", dbHost);

  //
  // Initialize libmongoc's internals
  //
  mongoc_init();

  //
  // Safely create a MongoDB URI object from the given string
  //
  LM_TMP(("DB: Mongo URI: '%s'", mongoUri));
  orionldState.mongoUri = mongoc_uri_new_with_error(mongoUri, &mongoError);
  if (orionldState.mongoUri == NULL)
    LM_X(1, ("mongoc_uri_new_with_error(%s): %s", orionldState.mongoUri, mongoError.message));

  //
  // Create a new client instance
  //
  LM_TMP(("DB: Creating a new client instance"));
  orionldState.mongoClient = mongoc_client_new_from_uri(orionldState.mongoUri);
  if (orionldState.mongoClient == NULL)
    LM_X(1, ("mongoc_client_new_from_uri failed"));
  LM_TMP(("DB: Got a new client instance"));

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
  mongoEntitiesCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "entities");
  if (mongoEntitiesCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'entities') failed", dbName));
  LM_TMP(("DB: entities collection handle OK!"));

  mongoRegistrationsCollectionP = mongoc_client_get_collection(orionldState.mongoClient, dbName, "registrations");
  if (mongoRegistrationsCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection(%s, 'regiatrations') failed", dbName));
  LM_TMP(("DB: registrations collection handle OK"));
}
