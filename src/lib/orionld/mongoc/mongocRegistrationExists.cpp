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
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocRegistrationExists.h"             // Own interface



// -----------------------------------------------------------------------------
//
// mongocRegistrationExists -
//
bool mongocRegistrationExists(const char* registrationId, bool* foundP)
{
  bson_t            mongoFilter;
  const bson_t*     mongoDocP;
  mongoc_cursor_t*  mongoCursorP;
  bson_error_t      mongoError;

  *foundP = false;

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);
  bson_append_utf8(&mongoFilter, "_id", 3, registrationId, -1);

  mongocConnectionGet(orionldState.tenantP, DbRegistrations);

  //
  // Run the query
  //
  // semTake(&mongoRegistrationsSem);
  if ((mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.registrationsP, &mongoFilter, NULL, NULL)) == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts ERROR", 500);
    return false;
  }

  while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
  {
    *foundP = true;
    break;
  }

  if (mongoc_cursor_error(mongoCursorP, &mongoError))
  {
    orionldError(OrionldInternalError, "Database Error", mongoError.message, 500);
    return false;
  }

  mongoc_cursor_destroy(mongoCursorP);
  // semGive(&mongoRegistrationsSem);
  bson_destroy(&mongoFilter);

  return true;
}
