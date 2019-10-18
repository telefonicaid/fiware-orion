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
#include "kjson/kjRender.h"                                      // kjRender - TMP
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/db/dbNameGet.h"                                // dbNameGet



// -----------------------------------------------------------------------------
//
// mongocEntityLookup -
//
KjNode* mongocEntityLookup(const char* entityId)
{    
  char              dbName[512];
  bson_t            mongoFilter;
  const bson_t*     mongoDocP;
  mongoc_cursor_t*  mongoCursorP;
  bson_error_t      mongoError;
  char*             title;
  char*             details;
  KjNode*           entityNodeP = NULL;
  
  LM_TMP(("DB: looking up entity with id: '%s'", entityId));

  LM_TMP(("DB: Composing collection name"));
  if (dbNameGet(dbName, sizeof(dbName)) == -1)
    return NULL;
  LM_TMP(("DB: database name: %s", dbName));

  //
  // Create the filter for the query
  //
  LM_TMP(("DB: Calling bson_append_utf8"));
  bson_append_utf8(&mongoFilter, "_id.id", 3, entityId, -1);
  LM_TMP(("DB: After bson_append_utf8"));

  //
  // Run the query
  //
  // semTake(&mongoEntitiesSem);
  LM_TMP(("DB: Calling mongoc_client_command_simple"));
  if ((mongoCursorP = mongoc_collection_find_with_opts(mongoEntitiesCollectionP, &mongoFilter, NULL, NULL)) == NULL)
  {
    LM_E(("mongoc_collection_find_with_opts ERROR"));
    return NULL;
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    LM_TMP(("DB: After mongoc_cursor_next - calling dbDataToKjTree"));
    entityNodeP = dbDataToKjTree(mongoDocP, &title, &details);
    LM_TMP(("DB: back from dbDataToKjTree, entityNodeP at %p", entityNodeP));
    break;  // Just using the first one - should be no more than one!
  }

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    LM_W(("DB Error (%s)", mongoError.message));
    return NULL;
  }

  mongoc_cursor_destroy(mongoCursorP);
  // semGive(&mongoEntitiesSem);
  bson_destroy(&mongoFilter);
 
  return entityNodeP;
}
