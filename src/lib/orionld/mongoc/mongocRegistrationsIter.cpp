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
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_RLOG
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocRegistrationsIter.h"              // RegCacheIterFunc



// -----------------------------------------------------------------------------
//
// mongocRegistrationsIter -
//
int mongocRegistrationsIter(RegCache* rcP, RegCacheIterFunc callback)
{
  bson_t                mongoFilter;
  const bson_t*         mongoDocP = NULL;
  mongoc_cursor_t*      mongoCursorP;
  bson_error_t          mongoError;
  mongoc_read_prefs_t*  readPrefs = mongoc_read_prefs_new(MONGOC_READ_NEAREST);
  char*                 title;
  char*                 details;
  int                   retVal    = 0;

  //
  // Empty filter for the query - we want ALL registrations
  //
  bson_init(&mongoFilter);

  mongocConnectionGet(NULL, DbNone);

  //
  // Can't use the "orionldState.mongoc.registrationsP" here, as we'll deal with not one single tenant but all of them
  //
  mongoc_collection_t* regsCollectionP = mongoc_client_get_collection(orionldState.mongoc.client, rcP->tenantP->mongoDbName, "registrations");

  if (regsCollectionP == NULL)
    LM_X(1, ("mongoc_client_get_collection failed for 'registrations' collection on tenant '%s'", rcP->tenantP->mongoDbName));

  //
  // Run the query
  //
  MONGOC_RLOG("Query for all regs", rcP->tenantP->mongoDbName, "registrations", NULL, NULL, LmtMongoc);
  mongoCursorP = mongoc_collection_find_with_opts(regsCollectionP, &mongoFilter, NULL, readPrefs);
  if (mongoCursorP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts ERROR", 500);
    mongoc_read_prefs_destroy(readPrefs);
    bson_destroy(&mongoFilter);
    return 1;
  }

  int hits = 0;
  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    char* json = bson_as_relaxed_extended_json(mongoDocP, NULL);
    LM_T(LmtMongoc, ("Found a registration in the DB: '%s'", json));
    bson_free(json);

    KjNode* dbRegP = mongocKjTreeFromBson(mongoDocP, &title, &details);
    if (dbRegP == NULL)
    {
      orionldError(OrionldInternalError, "Database Error", "unable to convert DB-Model registration to API Format", 500);
      continue;
    }

    if (callback(rcP, dbRegP) != 0)
    {
      retVal = 2;
      break;
    }

    ++hits;
  }

  LM_T(LmtMongoc, ("Found %d hits in the db", hits));

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    retVal = 3;
    orionldError(OrionldInternalError, "Database Error", mongoError.message, 500);
  }

  mongoc_collection_destroy(regsCollectionP);
  mongoc_cursor_destroy(mongoCursorP);
  mongoc_read_prefs_destroy(readPrefs);
  bson_destroy(&mongoFilter);

  return retVal;
}
