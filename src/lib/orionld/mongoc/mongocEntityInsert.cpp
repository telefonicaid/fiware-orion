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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_WLOG
#include "orionld/mongoc/mongocEntityInsert.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocEntityInsert -
//
// For now, only POST /entities uses this function
//
bool mongocEntityInsert(KjNode* dbEntityP, const char* entityId)
{
  bson_t document;
  bson_t reply;

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  bson_init(&document);
  bson_init(&reply);

  mongocKjTreeToBson(dbEntityP, &document);

  MONGOC_WLOG("Creating Entity", orionldState.tenantP->mongoDbName, "entities", NULL, &document, LmtMongoc);
  bool b = mongoc_collection_insert_one(orionldState.mongoc.entitiesP, &document, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("mongoc error inserting entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
  }

  // mongocConnectionRelease(); - done at the end of the request - the connection is needed for Subs, Regs, ...

  bson_destroy(&document);
  bson_destroy(&reply);

  return b;
}
