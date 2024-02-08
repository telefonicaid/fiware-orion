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
#include <bson/bson.h>                                             // bson_t, ...
#include <mongoc/mongoc.h>                                         // MongoDB C Client Driver


extern "C"
{
#include "kjson/KjNode.h"                                          // KjNode
#include "kjson/kjBuilder.h"                                       // kjArray
}

#include "logMsg/logMsg.h"                                         // LM_*

#include "orionld/common/orionldState.h"                           // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                    // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                   // mongocKjTreeFromBson
#include "orionld/mongoc/mongocEntityTypesFromRegistrationsGet.h"  // Own interface



// -----------------------------------------------------------------------------
//
// FIXME: Move these two functions elsewhere
//
// They are implemented in mongoCppLegacyEntityTypesFromRegistrationsGet.cpp
// But, they don't belong there ...
//
extern void typeExtract(KjNode* regArray, KjNode* typeArray);
extern void entitiesAndPropertiesExtract(KjNode* regArray, KjNode* typeArray);



// -----------------------------------------------------------------------------
//
// mongocEntityTypesFromRegistrationsGet -
//
KjNode* mongocEntityTypesFromRegistrationsGet(bool details)
{
  //
  // Projection and options
  //
  bson_t projection;
  bson_init(&projection);

  bson_append_bool(&projection, "contextRegistration.entities.type", 33, true);

  if (details)
    bson_append_bool(&projection, "contextRegistration.attrs", 25, true);

  bson_t options;
  bson_init(&options);
  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  // Connection
  mongocConnectionGet(orionldState.tenantP, DbRegistrations);

  //
  // Run the query
  //
  mongoc_cursor_t*      mongoCursorP;
  bson_error_t          mongoError;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);
  bson_t                mongoFilter;

  bson_init(&mongoFilter);

  if ((mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.registrationsP, &mongoFilter, &options, readPrefs)) == NULL)
  {
    LM_E(("Internal Error (mongoc_collection_find_with_opts ERROR)"));
    mongoc_read_prefs_destroy(readPrefs);
    bson_destroy(&options);
    bson_destroy(&mongoFilter);
    return NULL;
  }

  KjNode*        kjRegArray        = NULL;
  KjNode*        registrationNodeP = NULL;
  const bson_t*  mongoDocP;

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    char* title;
    char* detail;

    registrationNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);
    if (registrationNodeP == NULL)
      LM_E(("%s: %s", title, detail));
    else
    {
      if (kjRegArray == NULL)
        kjRegArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(kjRegArray, registrationNodeP);
    }
  }

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    LM_E(("Internal Error (DB Error '%s')", mongoError.message));
    bson_destroy(&options);
    mongoc_cursor_destroy(mongoCursorP);
    mongoc_read_prefs_destroy(readPrefs);
    bson_destroy(&mongoFilter);
    return NULL;
  }

  bson_destroy(&options);
  bson_destroy(&mongoFilter);
  mongoc_cursor_destroy(mongoCursorP);
  mongoc_read_prefs_destroy(readPrefs);

  // FIXME: This part has nothing to do with DB - move out from the database libs
  KjNode* typeArray = NULL;
  if (kjRegArray != NULL)
  {
    typeArray = kjArray(orionldState.kjsonP, NULL);
    if (details == false)
      typeExtract(kjRegArray, typeArray);
    else
      entitiesAndPropertiesExtract(kjRegArray, typeArray);
  }

  return typeArray;
}
