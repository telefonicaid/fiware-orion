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
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocEntitiesDelete.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// mongocEntitiesDelete -
//
bool mongocEntitiesDelete(KjNode* entityIdArray)
{
  mongocConnectionGet(orionldState.tenantP, DbEntities);

  mongoc_bulk_operation_t* bulkP;

  bulkP = mongoc_collection_create_bulk_operation_with_opts(orionldState.mongoc.entitiesP, NULL);

  for (KjNode* idNodeP = entityIdArray->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    bson_t selector;
    bson_init(&selector);
    bson_append_utf8(&selector, "_id.id", 6, idNodeP->value.s, -1);

    mongoc_bulk_operation_remove(bulkP, &selector);
    bson_destroy(&selector);
  }

  bson_error_t error;
  bson_t       reply;
  bool r = mongoc_bulk_operation_execute(bulkP, &reply, &error);

  if (r == false)
  {
    char* errorString = bson_as_canonical_extended_json(&reply, NULL);
    LM_E(("mongoc_bulk_operation_execute: %s", errorString));
    bson_free(errorString);
  }

  bson_destroy(&reply);
  mongoc_bulk_operation_destroy(bulkP);

  return r;
}
