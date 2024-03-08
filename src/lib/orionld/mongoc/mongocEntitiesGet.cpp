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

extern "C"
{
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntitiesGet.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// mongocEntitiesGet -
//
KjNode* mongocEntitiesGet(char** fieldV, int fields, bool entityIdPresent)
{
  const bson_t*         mongoDocP;
  mongoc_cursor_t*      mongoCursorP;
  KjNode*               entityNodeP = NULL;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);

  //
  // Sort, Limit, Offset
  //
  bson_t options;
  bson_t sortDoc;
  int    limit       = orionldState.uriParams.limit;
  int    offset      = orionldState.uriParams.offset;

  bson_init(&options);
  bson_init(&sortDoc);

  bson_append_int32(&sortDoc, "creDate", 7, 1);
  bson_append_int32(&sortDoc, "_id.id", 6, 1);
  bson_append_document(&options, "sort", 4, &sortDoc);
  bson_destroy(&sortDoc);

  bson_append_int32(&options, "limit", 5, limit);
  if (offset != 0)
    bson_append_int32(&options, "skip", 4, offset);


  //
  // Projection (will be added to if attrList != NULL)
  //
  bson_t projection;
  bson_init(&projection);
  if (entityIdPresent == true)
  {
    bson_append_bool(&projection, "_id.id",   6, true);
    bson_append_bool(&projection, "_id.type", 8, true);
  }
  else
    bson_append_bool(&projection, "_id", 3, false);

  for (int ix = 0; ix < fields; ix++)
  {
    bson_append_bool(&projection, fieldV[ix], -1, true);
  }

  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  bson_t mongoFilter;
  bson_init(&mongoFilter);

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  //
  // Run the query
  //
  mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs);
  bson_destroy(&options);

  if (mongoCursorP == NULL)
  {
    LM_E(("Database Error (mongoc_collection_find_with_opts ERROR)"));
    bson_destroy(&mongoFilter);
    mongoc_read_prefs_destroy(readPrefs);
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts failed", 500);
    return NULL;
  }

  int      hits        = 0;
  KjNode*  entityArray = NULL;

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    char* title;
    char* detail;

    entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);

    if (entityNodeP != NULL)
    {
      if (entityArray == NULL)
        entityArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(entityArray, entityNodeP);
      ++hits;
    }
    else
      LM_E(("Database Error (%s: %s)", title, detail));
  }

  bson_error_t error;
  if (mongoc_cursor_error(mongoCursorP, &error) == true)
    LM_E(("mongoc_cursor_error: %d.%d: '%s'", error.domain, error.code, error.message));

  mongoc_cursor_destroy(mongoCursorP);
  mongoc_read_prefs_destroy(readPrefs);

  return entityArray;
}
