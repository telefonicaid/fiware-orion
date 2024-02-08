/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntityTypeGet.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// kjAttributesWithTypeExtract - FIXME: Move to kjTree/kjAttributesWithTypeExtract.h/cpp
//
extern bool kjAttributesWithTypeExtract(KjNode* kjTree, KjNode* entityP);



// -----------------------------------------------------------------------------
//
// mongocEntityTypeGet -
//
KjNode* mongocEntityTypeGet(OrionldProblemDetails* pdP, const char* typeLongName, int* noOfEntitiesP)
{
  KjNode*                               outArray       = kjArray(orionldState.kjsonP, NULL);
  int                                   entities       = 0;

  bson_t mongoFilter;
  bson_init(&mongoFilter);
  bson_append_utf8(&mongoFilter, "_id.type", 8, typeLongName, -1);

  //
  // We only want the "attrs" field back
  //
  bson_t options;
  bson_t projection;

  bson_init(&options);
  bson_init(&projection);
  bson_append_bool(&projection, "_id",   3, false);
  bson_append_bool(&projection, "attrs", 5, true);

  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  mongoc_read_prefs_t* readPrefs = mongoc_read_prefs_new(MONGOC_READ_NEAREST);

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  mongoc_cursor_t*      mongoCursorP;
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

  const bson_t* mongoDocP;
  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    char*   title;
    char*   detail;
    KjNode* kjTree  = mongocKjTreeFromBson(mongoDocP, &title, &detail);

    if (kjTree == NULL)
    {
      LM_E(("%s: %s", title, detail));
      continue;
    }

    KjNode* entityP = kjObject(orionldState.kjsonP, NULL);
    if (kjAttributesWithTypeExtract(kjTree, entityP) == true)
    {
      ++entities;
      kjChildAdd(outArray, entityP);
    }
    else
      LM_E(("Internal Error (kjAttributesWithTypeExtract failed)"));
  }

  mongoc_read_prefs_destroy(readPrefs);
  mongoc_cursor_destroy(mongoCursorP);

  *noOfEntitiesP = entities;

  return outArray;
}
