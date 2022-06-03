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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntitiesQuery.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// entityTypeFilter -
//
static void entityTypeFilter(bson_t* mongoFilterP, StringArray* entityTypes)
{
  if (entityTypes->items == 1)  // Just a single type?
    bson_append_utf8(mongoFilterP, "_id.type", 8, entityTypes->array[0], -1);
  else
  {
    bson_t in;
    bson_t entityTypeArray;

    bson_init(&in);
    bson_init(&entityTypeArray);

    for (int ix = 0; ix < entityTypes->items; ix++)
    {
      char num[32];
      int  numLen;

      if (ix < 10)
      {
        num[0] = '0' + ix;
        num[1] = 0;
        numLen = 1;
      }
      else
      {
        num[0] = '0' + ix % 10;
        num[1] = '0' + ix / 10;
        num[2] = 0;
        numLen = 2;
      }

      LM_TMP(("Appending type '%s' ix:%s", entityTypes->array[ix], num));
      bson_append_utf8(&entityTypeArray, num, numLen, entityTypes->array[ix], -1);
    }
    bson_append_array(&in, "$in", 3, &entityTypeArray);
    bson_append_document(mongoFilterP, "_id.type", 8, &in);

    bson_destroy(&in);                // It's safe to destroy once incorporated into mongoFilterP
    bson_destroy(&entityTypeArray);   // It's safe to destroy once incorporated into mongoFilterP
  }
}



// -----------------------------------------------------------------------------
//
// attributesFilter -
//
static void attributesFilter(bson_t* mongoFilterP, StringArray* attributes)
{
  char   path[512];
  bson_t exists;

  bson_init(&exists);
  bson_append_int32(&exists, "$exists", 7, 1);

  if (attributes->items == 1)  // Just a single attribute?
  {
    // { "attrs.<eq-attr-name>": { "$exists": 1 } }

    int len = snprintf(path, sizeof(path) - 1, "attrs.%s", attributes->array[0]);
    dotForEq(&path[6]);
    LM_TMP(("path: %s", path));
    bson_append_document(mongoFilterP, path, len, &exists);
  }
  else
  {
    // { $or: [ { attrs.A1: {$exists: 1} }, { attrs.A2: {$exists: 1} }, { attrs.A3: {$exists: 1} } ] }
    bson_t array;
    bson_init(&array);

    char num[3] = { '0', '0', 0 };
    int  numLen = 1;

    for (int ix = 0; ix < attributes->items; ix++)
    {
      int    len = snprintf(path, sizeof(path) - 1, "attrs.%s", attributes->array[ix]);
      bson_t attrExists;

      bson_init(&attrExists);
      dotForEq(&path[6]);
      LM_TMP(("path: %s", path));
      bson_append_document(&attrExists, path, len, &exists);

      LM_TMP(("KZ: Adding attr '%s' at index '%s', indexLen: %d", path, &num[2-numLen], numLen));
      bson_append_document(&array, &num[2-numLen], numLen, &attrExists);

      num[1] += 1;
      if (((ix + 1) % 10) == 0)
      {
        num[1] = '0';
        num[0] += 1;
        numLen = 2;
      }

      bson_destroy(&attrExists);
    }

    bson_append_array(mongoFilterP, "$or", 3, &array);
  }
}



// -----------------------------------------------------------------------------
//
// mongocEntitiesQuery -
//
// Parameters passed via orionldState:
// - limit
// - offset
// - count
//
KjNode* mongocEntitiesQuery
(
  StringArray* entityTypes,
  StringArray* attributes,
  int64_t*     countP
)
{
  if (attributes->items > 99)
  {
    orionldError(OrionldBadRequestData, "Too many attributes", "maximum is 99", 400);
    return NULL;
  }

  bson_t                mongoFilter;
  const bson_t*         mongoDocP;
  mongoc_cursor_t*      mongoCursorP;
  char*                 title;
  char*                 detail;
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
  bson_append_document(&options, "sort", 4, &sortDoc);

  bson_append_int32(&options, "limit", 5, limit);
  if (offset != 0)
    bson_append_int32(&options, "skip", 4, offset);

  LM_TMP(("LIMIT:  %d", limit));
  LM_TMP(("OFFSET: %d", offset));

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);

  // Entity Types
  if ((entityTypes != NULL) && (entityTypes->items > 0))
    entityTypeFilter(&mongoFilter, entityTypes);

  if ((attributes != NULL) && (attributes->items > 0))
    attributesFilter(&mongoFilter, attributes);

  mongocConnectionGet();

  if (orionldState.mongoc.entitiesP == NULL)
    orionldState.mongoc.entitiesP = mongoc_client_get_collection(orionldState.mongoc.client, orionldState.tenantP->mongoDbName, "entities");

  // semTake(&mongoEntitiesSem);

  // count?
  if (orionldState.uriParams.count == true)
  {
    bson_error_t error;

    *countP = mongoc_collection_count_documents(orionldState.mongoc.entitiesP, &mongoFilter, NULL, readPrefs, NULL, &error);
    if (*countP == -1)
    {
      *countP = 0;
      LM_E(("Database Error (error counting entities: %d.%d: %s)", error.domain, error.code, error.message));
    }
  }

  //
  // Run the query
  //
  KjNode* entityArray = kjArray(orionldState.kjsonP, NULL);

  if (limit != 0)
  {
    if ((mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs)) == NULL)
    {
      LM_E(("Database Error (mongoc_collection_find_with_opts ERROR)"));
      mongoc_read_prefs_destroy(readPrefs);
      return NULL;
    }

    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);
      if (entityNodeP != NULL)
        kjChildAdd(entityArray, entityNodeP);
      else
        LM_E(("Database Error (%s: %s)", title, detail));
    }

    mongoc_cursor_destroy(mongoCursorP);
  }

  // semGive(&mongoEntitiesSem);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  return entityArray;
}
