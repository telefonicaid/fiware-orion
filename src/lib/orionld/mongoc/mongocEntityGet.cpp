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
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

#include "logMsg/logMsg.h"                                       // LM_*

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntityGet.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// mongocEntityGet -
//
KjNode* mongocEntityGet(const char* entityId, const char** projectionV, bool includeEntityId)
{
  bson_t        mongoFilter;
  const bson_t* mongoDocP = NULL;
  bson_t        options;

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);
  bson_append_utf8(&mongoFilter, "_id.id", 6, entityId, -1);

  //
  // Prepare 'options'
  //
  bson_init(&options);

  if (projectionV != NULL)
  {
    //
    // Populate the projection
    //
    bson_t projection;
    bson_init(&projection);

    int projections = 0;

    while (projectionV[projections] != NULL)
    {
      bson_append_bool(&projection, projectionV[projections], -1, true);
      ++projections;
    }

    if (includeEntityId)
      bson_append_bool(&projection, "_id.id", 6, true);
    else
      bson_append_bool(&projection, "_id", 3, false);

    if (projections > 0)
      bson_append_document(&options, "projection", 10, &projection);

    bson_destroy(&projection);
  }

  //
  // Get the connection
  //
  mongocConnectionGet(orionldState.tenantP, DbEntities);

  //
  // Perform the query
  //
  KjNode*               entityNodeP = NULL;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);
  mongoc_cursor_t*      mongoCursorP;

  mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs);
  bson_destroy(&options);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  if (mongoCursorP == NULL)
  {
    LM_E(("Database Error (mongoc_collection_find_with_opts ERROR)"));
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts failed", 500);
    return NULL;
  }

  if (mongoc_cursor_next(mongoCursorP, &mongoDocP) == false)
  {
    mongoc_cursor_destroy(mongoCursorP);
    return NULL;  // Not Found
  }

  char* title  = (char*) "title";
  char* detail = (char*) "detail";
  entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);
  if (entityNodeP == NULL)
    LM_E(("mongocKjTreeFromBson: %s: %s", title, detail));

  mongoc_cursor_destroy(mongoCursorP);

  return entityNodeP;
}
