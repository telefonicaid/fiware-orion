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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "orionld/common/orionldState.h"                         // orionldState, dbName
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocTenantExists.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocTenantExists -
//
//
// https://www.mongodb.com/docs/manual/reference/command:
//   mongoc_client_command_with_opts(client, "admin", BSON({listDatabases: 1})) ?
//
// bool mongoc_client_read_command_with_opts
// (
//   mongoc_client_t*            client,
//   const char*                 db_name,
//   const bson_t*               command,
//   const mongoc_read_prefs_t*  read_prefs,
//   const bson_t*               opts,
//   bson_t*                     reply,
//   bson_error_t*               error
// );
//
bool mongocTenantExists(const char* tenantName)
{
  bson_t        command;
  bson_t        reply;
  bson_error_t  mcError;

  bson_init(&command);
  bson_init(&reply);

  mongocConnectionGet(NULL, DbNone);

  bson_append_int32(&command, "listDatabases", 13, 1);

  bool b = mongoc_client_read_command_with_opts(orionldState.mongoc.client, "admin", &command, NULL, NULL, &reply, &mcError);
  if (b == false)
    LM_RE(false, ("Database Error (%s)", mcError.message));

  char*   title;
  char*   detail;
  KjNode* responseP = mongocKjTreeFromBson(&reply, &title, &detail);

  // Once I have the tree I can free up the bson buffers
  bson_destroy(&command);
  bson_destroy(&reply);

  if (responseP == NULL)
    return false;

  KjNode* databasesP = kjLookup(responseP, "databases");
  if (databasesP == NULL)
    return false;

  char fullDbName[64];
  snprintf(fullDbName, sizeof(fullDbName) - 1, "%s-%s", dbName, tenantName);

  for (KjNode* dbP = databasesP->value.firstChildP; dbP != NULL; dbP = dbP->next)
  {
    KjNode* nameP = kjLookup(dbP, "name");

    if (nameP == NULL)
      continue;
    if (nameP->type != KjString)
      continue;

    if (strcmp(nameP->value.s, fullDbName) == 0)
      return true;
  }

  return false;
}
