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
#include "orionld/mongoc/mongocServerVersionGet.h"               // Own interface



// -----------------------------------------------------------------------------
//
// mongocServerVersionGet -
//
bool mongocServerVersionGet(char* serverVersionBuf)
{
  bson_t        command;
  bson_t        reply;
  bson_error_t  mcError;

  bson_init(&command);
  bson_init(&reply);

  mongocConnectionGet(NULL, DbNone);

  bson_append_int32(&command, "buildinfo", 9, 1);

  bool b = mongoc_client_read_command_with_opts(orionldState.mongoc.client, "admin", &command, NULL, NULL, &reply, &mcError);
  if (b == false)
    LM_RE(false, ("Database Error (%s)", mcError.message));

  char*   title;
  char*   detail;
  KjNode* responseP = mongocKjTreeFromBson(&reply, &title, &detail);

  bson_destroy(&command);
  bson_destroy(&reply);

  if (responseP == NULL)
    return false;

  KjNode* versionNodeP = kjLookup(responseP, "version");

  if ((versionNodeP == NULL) || (versionNodeP->type != KjString))
    return false;

  strcpy(serverVersionBuf, versionNodeP->value.s);

  return true;
}
