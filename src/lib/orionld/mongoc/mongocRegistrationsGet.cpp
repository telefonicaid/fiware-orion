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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocRegistrationsGet.h"               // Own interface



/* ****************************************************************************
*
* mongocRegistrationsGet -
*/
KjNode* mongocRegistrationsGet(int64_t* countP)
{
  bson_t               mongoFilter;
  const bson_t*        mongoDocP;
  mongoc_cursor_t*     mongoCursorP;
  bson_error_t         mongoError;
  mongoc_read_prefs_t* readPrefs = mongoc_read_prefs_new(MONGOC_READ_NEAREST);
  KjNode*              dbRegV    = kjArray(orionldState.kjsonP, NULL);
  char*                title;
  char*                details;

  //
  // Sort, Limit, Offset
  //
  bson_t options;
  bson_t sortDoc;
  int    limit       = orionldState.uriParams.limit;
  int    offset      = orionldState.uriParams.offset;

  bson_init(&options);
  bson_init(&sortDoc);

  bson_append_int32(&sortDoc, "createdAt", 9, 1);
  bson_append_document(&options, "sort", 4, &sortDoc);
  bson_destroy(&sortDoc);

  bson_append_int32(&options, "limit", 5, limit);
  if (offset != 0)
    bson_append_int32(&options, "skip", 4, offset);

  //
  // Empty filter for the query - we want ALL registrations
  //
  bson_init(&mongoFilter);

  mongocConnectionGet(orionldState.tenantP, DbRegistrations);

  // count?
  if (orionldState.uriParams.count == true)
  {
    bson_error_t error;

    *countP = mongoc_collection_count_documents(orionldState.mongoc.registrationsP, &mongoFilter, NULL, readPrefs, NULL, &error);
    if (*countP == -1)
    {
      *countP = 0;
      LM_E(("Database Error (error counting entities: %d.%d: %s)", error.domain, error.code, error.message));
    }

    if (limit == 0)  // Only count requested
    {
      bson_destroy(&mongoFilter);
      mongoc_read_prefs_destroy(readPrefs);

      if (*countP == -1)
      {
        orionldError(OrionldInternalError, "Database Error", error.message, 500);
        return NULL;
      }

      return dbRegV;
    }
  }


  //
  // Run the query
  //
  mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.registrationsP, &mongoFilter, &options, readPrefs);
  if (mongoCursorP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts ERROR", 500);
    mongoc_read_prefs_destroy(readPrefs);
    bson_destroy(&mongoFilter);
    return NULL;
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    KjNode* dbRegP = mongocKjTreeFromBson(mongoDocP, &title, &details);

    if (dbRegP == NULL)
    {
      orionldError(OrionldInternalError, "Database Error", "unable to convert DB-Model registration to API Format", 500);
      continue;
    }

    kjChildAdd(dbRegV, dbRegP);
  }

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    orionldError(OrionldInternalError, "Database Error", mongoError.message, 500);
    dbRegV = NULL;
  }

  mongoc_cursor_destroy(mongoCursorP);
  mongoc_read_prefs_destroy(readPrefs);
  bson_destroy(&mongoFilter);

  return dbRegV;
}
