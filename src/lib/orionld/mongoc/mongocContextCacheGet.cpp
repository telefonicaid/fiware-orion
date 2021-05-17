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
#include <unistd.h>                                              // NULL
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // mongoc_cursor_t, ...

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd
#include "kjson/kjRender.h"                                      // TMP: kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // mongoContextsSem
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/contextCache/orionldContextCache.h"            // Own interface



// -----------------------------------------------------------------------------
//
// mongocContextCacheGet -
//
KjNode* mongocContextCacheGet(void)
{
  mongoc_cursor_t*  cursor;
  bson_t            bsonContext;
  const bson_t*     bsonContextP = &bsonContext;
  bson_t*           query        = bson_new();       // Empty - to find all the contexts in the DB
  KjNode*           contextArray = kjArray(orionldState.kjsonP, NULL);

  sem_wait(&mongoContextsSem);

  cursor = mongoc_collection_find_with_opts(mongoContextsCollectionP, query, NULL, NULL);

  int matches = 0;
  while (mongoc_cursor_next(cursor, &bsonContextP))
  {
    char*    title;
    char*    detail;
    KjNode*  contextNodeP = mongocKjTreeFromBson(bsonContextP, &title, &detail);

    if (contextNodeP == NULL)
    {
      LM_E(("Database Error parsing retrieved contexts (%s: %s)", title, detail));
      continue;
    }

    kjChildAdd(contextArray, contextNodeP);
    ++matches;
  }
  sem_post(&mongoContextsSem);

  bson_destroy(query);
  mongoc_cursor_destroy(cursor);


  return contextArray;
}
