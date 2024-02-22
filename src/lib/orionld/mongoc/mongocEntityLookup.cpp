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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocAuxAttributesFilter.h"            // mongocAuxAttributesFilter
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_RLOG
#include "orionld/mongoc/mongocEntityLookup.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocEntityLookup -
//
// This function, that seems quite similar to mongocEntityRetrieve, is used by:
//   * orionldPutEntity    - Uses creDate + attrs to make sure no attr types are modified
//   * orionldPatchEntity  - Like PUT but also using entity type, entire attributes, etc.
//   * orionldPostEntities - Only to make sure the entity does not already exists (mongocEntityExists should be implemented and used instead)
//   * orionldPostEntity   - The entire DB Entity is needed as it is later used as base for the "merge" with the payload body
//   * orionldGetEntity    - filtering over attributes (?attrs=A1,A2,...An&?geometryProperty=GP)
//
// So, this function is QUITE NEEDED, just as it is.
//
// The other one, mongocEntityRetrieve, does much more than just DB. It needs to be be REMOVED.
// mongocEntityRetrieve is only used by legacyGetEntity() which is being deprecated anyway.
//
KjNode* mongocEntityLookup(const char* entityId, const char* entityType, StringArray* attrsV, const char* geojsonGeometry, char** detailP)
{
  bson_t                mongoFilter;
  const bson_t*         mongoDocP;
  mongoc_cursor_t*      mongoCursorP;
  bson_error_t          mcError;
  char*                 title;
  char*                 details;
  KjNode*               entityNodeP = NULL;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);

  bson_append_utf8(&mongoFilter, "_id.id", 6, entityId, -1);

  if (entityType != NULL)
    bson_append_utf8(&mongoFilter, "_id.type", 8, entityType, -1);

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  //
  // Projection (will be added to if attrList != NULL)
  //
  bson_t projection;
  bson_init(&projection);
  bson_append_bool(&projection, "attrNames",       9, true);
  bson_append_bool(&projection, "creDate",         7, true);
  bson_append_bool(&projection, "modDate",         7, true);
  bson_append_bool(&projection, "lastCorrelator", 14, true);

  // Attribute List AND GeoJSON Geometry
  if ((attrsV != NULL) && (attrsV->items > 0))
  {
    if (mongocAuxAttributesFilter(&mongoFilter, attrsV, &projection, geojsonGeometry, false) == false)
    {
      if (detailP != NULL)
        *detailP = (char*) "mongocAuxAttributesFilter failed";
      return NULL;
    }
  }
  else
  {
    bson_append_bool(&projection, "attrs",     5, true);
    bson_append_bool(&projection, "@datasets", 9, true);
  }

  bson_t options;
  bson_init(&options);
  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  //
  // Run the query
  //
  MONGOC_RLOG("Entity Lookup", orionldState.tenantP->mongoDbName, "entities", &mongoFilter, &options, LmtMongoc);
  if ((mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs)) == NULL)
  {
    LM_E(("Internal Error (mongoc_collection_find_with_opts ERROR)"));
    entityNodeP = NULL;
    if (detailP != NULL)
      *detailP = (char*) "mongoc_collection_find_with_opts failed";
    goto done;
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &details);
    break;  // Just using the first one - should be no more than one!
  }

  if (mongoc_cursor_error(mongoCursorP, &mcError))
  {
    LM_E(("Internal Error (DB Error '%s')", mcError.message));
    if (detailP != NULL)
      *detailP = mcError.message;
    entityNodeP = NULL;
    goto done;
  }

 done:
  bson_destroy(&options);
  mongoc_read_prefs_destroy(readPrefs);
  mongoc_cursor_destroy(mongoCursorP);
  bson_destroy(&mongoFilter);

  return entityNodeP;
}
