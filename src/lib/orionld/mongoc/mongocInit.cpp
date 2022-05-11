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

#include "orionld/common/orionldState.h"                         // orionldState, mongocPool, ...
#include "orionld/common/tenantList.h"                           // tenant0 - the default tenant
#include "orionld/mongoc/mongocTenantsGet.h"                     // mongocTenantsGet
#include "orionld/mongoc/mongocGeoIndexInit.h"                   // mongocGeoIndexInit
#include "orionld/mongoc/mongocIdIndexCreate.h"                  // mongocIdIndexCreate
#include "orionld/mongoc/mongocInit.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// mongocInit -
//
void mongocInit(const char* dbHost, const char* dbName)
{
  bson_error_t mongoError;
  char         mongoUri[512];

  if ((dbUser[0] != 0) && (dbPwd[0] != 0))
    snprintf(mongoUri, sizeof(mongoUri), "mongodb://%s:%s@%s", dbUser, dbPwd, dbHost);
  else
    snprintf(mongoUri, sizeof(mongoUri), "mongodb://%s", dbHost);

  LM_K(("Connecting to mongo for the C driver, with URI '%s'", mongoUri));

  //
  // Initialize libmongoc's internals
  //
  mongoc_init();

  //
  // Safely create a MongoDB URI object from the given string
  //
  mongocUri = mongoc_uri_new_with_error(mongoUri, &mongoError);
  if (mongocUri == NULL)
  {
    if      ((dbUser[0] != 0) && (dbPwd[0] != 0))      LM_W(("Database username: '%s', password: '%s'", dbUser, dbPwd));
    else if  (dbUser[0] != 0)                          LM_W(("Database username given but no database password"));
    else if  (dbPwd[0]  != 0)                          LM_W(("Database password given but no database username"));

    LM_X(1, ("Unable to connect to mongo(URI: %s): %s", mongoUri, mongoError.message));
  }

  //
  // Initialize the connection pool
  //
  mongocPool = mongoc_client_pool_new(mongocUri);

  //
  // We want the newer, better, error handling
  //
  mongoc_client_pool_set_error_api(mongocPool, 2);


  //
  // Semaphore for the 'contexts' collection on DB 'orionld' - hopefully not needed in the end ...
  //
  sem_init(&mongocContextsSem, 0, 1);  // 0: shared between threads of the same process. 1: free to be taken

  if (mongocTenantsGet() == false)
    LM_X(1, ("Unable to extract tenants from the database - fatal error"));

  if (mongocGeoIndexInit() == false)
    LM_X(1, ("Unable to initialize geo indices in database - fatal error"));

  if (mongocIdIndexCreate(&tenant0) == false)
    LM_W(("Unable to create the index on Entity ID in the default database (%s)", dbName));
}
