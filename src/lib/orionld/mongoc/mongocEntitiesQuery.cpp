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
#include "orionld/q/QNode.h"                                     // QNode
#include "orionld/q/qTreeToBson.h"                               // qTreeToBson
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
  {
    bson_append_utf8(mongoFilterP, "_id.type", 8, entityTypes->array[0], -1);
    return;
  }

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

    bson_append_utf8(&entityTypeArray, num, numLen, entityTypes->array[ix], -1);
  }
  bson_append_array(&in, "$in", 3, &entityTypeArray);
  bson_append_document(mongoFilterP, "_id.type", 8, &in);

  bson_destroy(&in);                // It's safe to destroy once incorporated into mongoFilterP
  bson_destroy(&entityTypeArray);   // It's safe to destroy once incorporated into mongoFilterP
}



// -----------------------------------------------------------------------------
//
// entityIdFilter -
//
static void entityIdFilter(bson_t* mongoFilterP, StringArray* entityIds)
{
  if (entityIds->items == 1)  // Just a single id?
  {
    bson_append_utf8(mongoFilterP, "_id.id", 6, entityIds->array[0], -1);
    return;
  }

  bson_t in;
  bson_t entityIdArray;

  bson_init(&in);
  bson_init(&entityIdArray);

  for (int ix = 0; ix < entityIds->items; ix++)
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

    bson_append_utf8(&entityIdArray, num, numLen, entityIds->array[ix], -1);
  }
  bson_append_array(&in, "$in", 3, &entityIdArray);
  bson_append_document(mongoFilterP, "_id.id", 6, &in);

  bson_destroy(&in);                // It's safe to destroy once incorporated into mongoFilterP
  bson_destroy(&entityIdArray);     // It's safe to destroy once incorporated into mongoFilterP
}



// -----------------------------------------------------------------------------
//
// entityIdPatternFilter -
//
static void entityIdPatternFilter(bson_t* mongoFilterP, const char* idPattern)
{
  bson_append_regex(mongoFilterP, "_id.id", 6, idPattern, "m");
  LM_TMP(("Added REGEX for entity ID: '%s'", idPattern));
}



// -----------------------------------------------------------------------------
//
// attributesFilter -
//
static void attributesFilter(bson_t* mongoFilterP, StringArray* attrList, bson_t* projectionP)
{
  char   path[512];
  bson_t exists;

  bson_init(&exists);
  bson_append_int32(&exists, "$exists", 7, 1);

  if (attrList->items == 1)  // Just a single attribute?
  {
    // { "attrs.<eq-attr-name>": { "$exists": 1 } }

    int len = snprintf(path, sizeof(path) - 1, "attrs.%s", attrList->array[0]);
    dotForEq(&path[6]);

    bson_append_document(mongoFilterP, path, len, &exists);
    bson_append_bool(projectionP, path, len, true);
  }
  else
  {
    // { $or: [ { attrs.A1: {$exists: 1} }, { attrs.A2: {$exists: 1} }, { attrs.A3: {$exists: 1} } ] }
    bson_t array;
    bson_init(&array);

    char num[3] = { '0', '0', 0 };
    int  numLen = 1;

    for (int ix = 0; ix < attrList->items; ix++)
    {
      int    len = snprintf(path, sizeof(path) - 1, "attrs.%s", attrList->array[ix]);
      bson_t attrExists;

      bson_init(&attrExists);
      dotForEq(&path[6]);

      bson_append_document(&attrExists, path, len, &exists);
      bson_append_bool(projectionP, path, len, true);

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
    bson_destroy(&array);
  }

  bson_destroy(&exists);
}



// -----------------------------------------------------------------------------
//
// qFilter -
//
bool qFilter(bson_t* mongoFilterP, QNode* qNode)
{
  char* title;
  char* detail;

  if (qTreeToBson(qNode, mongoFilterP, &title, &detail) == false)
  {
    orionldError(OrionldInternalError, title, detail, 500);
    return false;
  }

  return true;
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
  StringArray*  entityTypeList,
  StringArray*  entityIdList,
  const char*   entityIdPattern,
  StringArray*  attrList,
  QNode*        qNode,
  int64_t*      countP
)
{
  if (attrList->items > 99)
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
  bson_destroy(&sortDoc);

  bson_append_int32(&options, "limit", 5, limit);
  if (offset != 0)
    bson_append_int32(&options, "skip", 4, offset);


  //
  // Projection (will be added to if attrList != NULL)
  //
  bson_t projection;
  bson_init(&projection);
  bson_append_bool(&projection, "_id.id",    6, true);
  bson_append_bool(&projection, "_id.type",  8, true);
  bson_append_bool(&projection, "creDate",   7, true);
  bson_append_bool(&projection, "modDate",   7, true);
  bson_append_bool(&projection, "@datasets", 9, true);


  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);

  // Entity Types
  if ((entityTypeList != NULL) && (entityTypeList->items > 0))
    entityTypeFilter(&mongoFilter, entityTypeList);

  // Entity IDs
  if ((entityIdList != NULL) && (entityIdList->items > 0))
    entityIdFilter(&mongoFilter, entityIdList);

  // Entity ID-Pattern
  if (entityIdPattern != NULL)
    entityIdPatternFilter(&mongoFilter, entityIdPattern);

  // Attribute List
  if ((attrList != NULL) && (attrList->items > 0))
    attributesFilter(&mongoFilter, attrList, &projection);
  else
    bson_append_bool(&projection, "attrs", 5, true);

  // Query Language
  if (qNode != NULL)
    qFilter(&mongoFilter, qNode);

  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

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
#if 1
    char* filterString  = bson_as_json(&mongoFilter, NULL);
    char* optionsString = bson_as_json(&options, NULL);
    LM_TMP(("Running the query with filter '%s'", filterString));
    LM_TMP(("Running the query with options '%s'", optionsString));
    bson_free(filterString);
    bson_free(optionsString);
#endif
    mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs);
    bson_destroy(&options);

    if (mongoCursorP == NULL)
    {
      LM_E(("Database Error (mongoc_collection_find_with_opts ERROR)"));
      bson_destroy(&mongoFilter);
      mongoc_read_prefs_destroy(readPrefs);
      return NULL;
    }

#if 0
    // <DEBUG>
    const bson_t* lastError = mongoc_collection_get_last_error(orionldState.mongoc.entitiesP);
    if (lastError != NULL)
      LM_E(("MongoC Error: %s", bson_as_canonical_extended_json(lastError, NULL)));
    // </DEBUG>
#endif

    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);

      if (entityNodeP != NULL)
      {
        kjTreeLog(entityNodeP, "entity to add to array");
        kjChildAdd(entityArray, entityNodeP);
      }
      else
        LM_E(("Database Error (%s: %s)", title, detail));
    }

    bson_error_t error;
    if (mongoc_cursor_error(mongoCursorP, &error) == true)
      LM_TMP(("mongoc_cursor_error: %d.%d.%s", error.domain, error.code, error.message));

    mongoc_cursor_destroy(mongoCursorP);
  }

  // semGive(&mongoEntitiesSem);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  return entityArray;
}
