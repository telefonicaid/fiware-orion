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
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocRegistrationLookup.h"             // Own interface



// -----------------------------------------------------------------------------
//
// mongocRegistrationLookup -
//
KjNode* mongocRegistrationLookup(const char* entityId, const char* attribute, int* noOfRegsP)
{
  if (noOfRegsP != NULL)
    *noOfRegsP = 0;

  //
  // Populate filter - on Entity ID and Attribute Name
  //
  bson_t mongoFilter;
  bson_init(&mongoFilter);

  bson_append_utf8(&mongoFilter, "contextRegistration.entities.id", 31, entityId, -1);

  if (attribute != NULL)
  {
    // { $or: [ { "contextRegistration.attrs": { $size: 0 }}, { "contextRegistration.attrs.name": 'attribute' } ]  }  ???
  }

  mongocConnectionGet(orionldState.tenantP, DbRegistrations);

  //
  // Run the query
  //
  // semTake(&mongoRegistrationsSem);
  //
  mongoc_cursor_t*  mongoCursorP;
  bson_error_t      mongoError;

  if ((mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.registrationsP, &mongoFilter, NULL, NULL)) == NULL)
  {
    LM_E(("Internal Error (mongoc_collection_find_with_opts ERROR)"));
    return NULL;
  }

  // mongocConnectionRelease(); - done at the end of the request

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
    return NULL;
  }

  mongoc_cursor_destroy(mongoCursorP);
  // semGive(&mongoRegistrationsSem);
  bson_destroy(&mongoFilter);

  return kjRegArray;
}
