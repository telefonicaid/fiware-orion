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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, ...
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldGeoInfo.h"                        // OrionldGeoInfo
#include "orionld/types/OrionldGeometry.h"                       // orionldGeometryFromString
#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_RLOG - FIXME: change name to mongocLog.h
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntitiesQuery2.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// qFilter - FIXME: Move to its own module
//
extern bool qFilter(bson_t* mongoFilterP, QNode* qNode);
extern bool geoFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP);



// -----------------------------------------------------------------------------
//
// entitySelectorFilter -
//
// if (idP)
//   db.entities.find({"_id.id": idP->value.s, "_id.type": typeP->value.s})
// else if (idPattern)
//   db.entities.find({"_id.id": REGEX, "_id.type": typeP->value.s})
// else
//   db.entities.find({"_id.type": typeP->value.s})
//
// All of those inside an $OR: [{}. {}, {}]
//
static bool entitySelectorFilter(bson_t* mongoFilterP, KjNode* entitySelectorList)
{
  // One single object in the Array?
  if (entitySelectorList->value.firstChildP->next == NULL)
  {
    KjNode* entitySelectorP = entitySelectorList->value.firstChildP;
    KjNode* idP             = kjLookup(entitySelectorP, "id");
    KjNode* typeP           = kjLookup(entitySelectorP, "type");
    KjNode* idPatternP      = (idP != NULL)? NULL : kjLookup(entitySelectorP, "idPattern");

    // Entity type is MANDATORY
    bson_append_utf8(mongoFilterP, "_id.type", 8, typeP->value.s, -1);

    if (idP != NULL)
      bson_append_utf8(mongoFilterP, "_id.id", 6, idP->value.s, -1);
    else if (idPatternP != NULL)
      bson_append_regex(mongoFilterP, "_id.id", 6, idPatternP->value.s, "m");

    return true;
  }

  bson_t array;
  bson_init(&array);

  int ix = 0;
  for (KjNode* entitySelectorP = entitySelectorList->value.firstChildP; entitySelectorP != NULL; entitySelectorP = entitySelectorP->next)
  {
    KjNode* idP        = kjLookup(entitySelectorP, "id");
    KjNode* typeP      = kjLookup(entitySelectorP, "type");
    KjNode* idPatternP = (idP == NULL)? NULL : kjLookup(entitySelectorP, "idPattern");
    bson_t  item;
    char    num[32];
    int     numLen;

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

    bson_init(&item);

    // Entity type is MANDATORY
    bson_append_utf8(&item, "_id.type", 8, typeP->value.s, -1);

    if (idP != NULL)
      bson_append_utf8(&item, "_id.id", 6, idP->value.s, -1);
    else if (idPatternP != NULL)
      bson_append_regex(&item, "_id.id", 6, idPatternP->value.s, "m");

    bson_append_document(&array, num, numLen, &item);
    bson_destroy(&item);
  }

  bson_append_array(mongoFilterP, "$or", 3, &array);
  bson_destroy(&array);

  return true;
}



// -----------------------------------------------------------------------------
//
// attributesFilter -
//
static bool attributesFilter(bson_t* mongoFilterP, KjNode* attrArray, bson_t* projectionP, const char* geojsonGeometry)
{
  char    path[512];  // Assuming 512 is always enough for the attribute path (attrs.$ATTR_LONG_NAME)
  bson_t  exists;
  bool    geojsonGeometryToProjection = (geojsonGeometry == NULL)? false : true;  // if GEOJSON, the "geometry" must be present

  bson_init(&exists);
  bson_append_int32(&exists, "$exists", 7, 1);

  //
  // Remember, an attribute may be either in "attrs", or "@datasets", or both ...
  // Example filter for attr A1 and A2:
  //
  // { $or: [ { attrs.A1: {$exists: 1} }, { attrs.A2: {$exists: 1} }, { @datasets.A1: {$exists: 1}}, { @datasets.A2: {$exists: 1}} ] }
  //

  char   num[3]   = { '0', '0', 0 };
  int    numLen   = 1;
  bson_t array;
  int    indexInArray  = 0;  // Need this for the order number in the array 'array'

  bson_init(&array);

  //
  // First "attrs.X"
  //
  for (KjNode* attrP = attrArray->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    int    len = snprintf(path, sizeof(path) - 1, "attrs.%s", attrP->value.s);
    bson_t attrExists;

    bson_init(&attrExists);
    dotForEq(&path[6]);

    bson_append_document(&attrExists, path, len, &exists);
    bson_append_bool(projectionP, path, len, true);

    bson_append_document(&array, &num[2-numLen], numLen, &attrExists);

    num[1] += 1;
    if (((indexInArray + 1) % 10) == 0)
    {
      num[1] = '0';
      num[0] += 1;
      numLen = 2;
    }

    bson_destroy(&attrExists);

    if ((geojsonGeometry != NULL) && (strcmp(attrP->value.s, geojsonGeometry) == 0))
    {
      geojsonGeometryToProjection = false;  // Already present - no need to add to projection
    }

    ++indexInArray;
  }

  //
  // Then "@datasets.X" - geojsonGeometryToProjection is already taken care of by "attrs" loop
  //
  for (KjNode* attrP = attrArray->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    int    len = snprintf(path, sizeof(path) - 1, "@datasets.%s", attrP->value.s);
    bson_t attrExists;

    bson_init(&attrExists);
    dotForEq(&path[10]);

    bson_append_document(&attrExists, path, len, &exists);
    bson_append_bool(projectionP, path, len, true);

    bson_append_document(&array, &num[2-numLen], numLen, &attrExists);

    num[1] += 1;
    if (((indexInArray + 1) % 10) == 0)
    {
      num[1] = '0';
      num[0] += 1;
      numLen = 2;
    }

    bson_destroy(&attrExists);
    ++indexInArray;
  }

  bson_append_array(mongoFilterP, "$or", 3, &array);
  bson_destroy(&array);

  if (geojsonGeometryToProjection == true)
  {
    int len = snprintf(path, sizeof(path) - 1, "attrs.%s", geojsonGeometry);
    dotForEq(&path[6]);
    bson_append_bool(projectionP, path, len, true);
    orionldState.geoPropertyFromProjection = true;

    // What about the dataset ... ?
  }

  bson_destroy(&exists);
  return true;
}



// -----------------------------------------------------------------------------
//
// mongocEntitiesQuery2 - needed for POST Query
//
// Four parameters are passed via orionldState:
// - orionldState.uriParams.offset  (URL parameter)
// - orionldState.uriParams.limit   (URL parameter)
// - orionldState.uriParams.count   (URL parameter)
// - orionldState.tenantP           (HTTP header)
//
KjNode* mongocEntitiesQuery2
(
  KjNode*          entitySelectorP,
  KjNode*          attrsArray,
  QNode*           qTree,
  OrionldGeoInfo*  geoInfoP,
  char*            lang,
  int64_t*         countP
)
{
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
  // Projection (will be added to if attrsArray != NULL)
  //
  bson_t projection;
  bson_init(&projection);
  bson_append_bool(&projection, "_id.id",          6, true);
  bson_append_bool(&projection, "_id.type",        8, true);
  bson_append_bool(&projection, "attrNames",       9, true);
  bson_append_bool(&projection, "creDate",         7, true);
  bson_append_bool(&projection, "modDate",         7, true);

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);
  if (entitySelectorP != NULL)
  {
    if (entitySelectorFilter(&mongoFilter, entitySelectorP) == false)
      return NULL;
  }

  char* geojsonGeometry = NULL;
  if (geoInfoP != NULL)
  {
    if (geoFilter(&mongoFilter, geoInfoP) == false)
      LM_RE(NULL, ("geoFilter flagged an error"));
    geojsonGeometry = geoInfoP->geoProperty;
  }

  // Attribute List
  if ((attrsArray != NULL) && (attrsArray->value.firstChildP != NULL))
  {
    if (attributesFilter(&mongoFilter, attrsArray, &projection, geojsonGeometry) == false)
      return NULL;
  }
  else
  {
    bson_append_bool(&projection, "attrs",     5, true);
    bson_append_bool(&projection, "@datasets", 9, true);
  }

  // Query Language
  if (qTree != NULL)
  {
    if (qFilter(&mongoFilter, qTree) == false)
      return NULL;
  }


  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  mongocConnectionGet();

  if (orionldState.mongoc.entitiesP == NULL)
    orionldState.mongoc.entitiesP = mongoc_client_get_collection(orionldState.mongoc.client, orionldState.tenantP->mongoDbName, "entities");

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
#if 0
    char* filterString  = bson_as_json(&mongoFilter, NULL);
    char* optionsString = bson_as_json(&options, NULL);

    bson_free(filterString);
    bson_free(optionsString);
#endif

    MONGOC_RLOG("Lookup Entities", orionldState.tenantP->mongoDbName, "entities", &mongoFilter, &options, LmtMongoc);
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

#if 1
    // <DEBUG>
    const bson_t* lastError = mongoc_collection_get_last_error(orionldState.mongoc.entitiesP);
    if (lastError != NULL)
      LM_E(("MongoC Error: %s", bson_as_canonical_extended_json(lastError, NULL)));
    // </DEBUG>
#endif

    int hits = 0;
    mongoDocP = NULL;
    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);

      if (entityNodeP != NULL)
      {
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
  }

  // semGive(&mongoEntitiesSem);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  return entityArray;
}
