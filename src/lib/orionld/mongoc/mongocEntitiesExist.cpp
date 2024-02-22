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
#include "kjson/kjBuilder.h"                                     // kjArray, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntitiesExist.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// entityIdFilter -
//
static void entityIdFilter(bson_t* mongoFilterP, KjNode* entityIdArray)
{
  if (entityIdArray->value.firstChildP->next == NULL)  // Just a single entity id?
  {
    bson_append_utf8(mongoFilterP, "_id.id", 6, entityIdArray->value.firstChildP->value.s, -1);
    return;
  }

  bson_t  in;
  bson_t  bsonEntityIdArray;

  bson_init(&in);
  bson_init(&bsonEntityIdArray);

  int ix = 0;
  for (KjNode* entyityIdP = entityIdArray->value.firstChildP; entyityIdP != NULL; entyityIdP = entyityIdP->next)
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

    bson_append_utf8(&bsonEntityIdArray, num, numLen, entyityIdP->value.s, -1);
    ++ix;
  }
  bson_append_array(&in, "$in", 3, &bsonEntityIdArray);
  bson_append_document(mongoFilterP, "_id.id", 6, &in);

  bson_destroy(&in);                 // It's safe to destroy once incorporated into mongoFilterP
  bson_destroy(&bsonEntityIdArray);  // It's safe to destroy once incorporated into mongoFilterP
}



// -----------------------------------------------------------------------------
//
// mongocEntitiesExist -
//
KjNode* mongocEntitiesExist(KjNode* entityIdArray, bool entityType)
{
  bson_t                mongoFilter;
  const bson_t*         mongoDocP;
  mongoc_cursor_t*      mongoCursorP;
  bson_t                projection;
  bson_t                options;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);

  bson_init(&projection);
  bson_append_bool(&projection, "_id.id", 6, true);

  if (entityType == true)
    bson_append_bool(&projection, "_id.type", 8, true);

  bson_init(&mongoFilter);
  entityIdFilter(&mongoFilter, entityIdArray);

  bson_init(&options);
  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs);
  bson_destroy(&options);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  if (mongoCursorP == NULL)
  {
    LM_E(("GEO: Database Error (mongoc_collection_find_with_opts ERROR)"));
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts failed", 500);
    return NULL;
  }

  KjNode* entityIdOutArray = kjArray(orionldState.kjsonP, NULL);
  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    char*    title;
    char*    detail;
    KjNode*  idNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);

    if (idNodeP != NULL)
      kjChildAdd(entityIdOutArray, idNodeP);
    else
      LM_E(("GEO: Database Error (%s: %s)", title, detail));
  }

  bson_error_t error;
  if (mongoc_cursor_error(mongoCursorP, &error) == true)
    LM_E(("GEO: mongoc_cursor_error: %d.%d: '%s'", error.domain, error.code, error.message));

  mongoc_cursor_destroy(mongoCursorP);

  return entityIdOutArray;
}
