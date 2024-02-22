/*
*
* Copyright 2023 FIWARE Foundation e.V.
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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // LmtMongoc

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_WLOG
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocAttributeReplace.h"               // Own interface



// -----------------------------------------------------------------------------
//
// mongocAttributeReplace -
//
bool mongocAttributeReplace(const char* entityId, KjNode* dbAttrP, char** detailP)
{
  mongocConnectionGet(orionldState.tenantP, DbEntities);

  bson_t selector;
  bson_init(&selector);
  bson_append_utf8(&selector, "_id.id", 6, entityId, -1);

  bson_t set;
  bson_t request;
  bson_t reply;

  bson_init(&request);
  bson_init(&reply);
  bson_init(&set);

  // Replace attrs.<attrNameInDbFormat>
  char   attrPath[512];
  strcpy(attrPath, "attrs.");
  strncpy(&attrPath[6], dbAttrP->name, 505);

  // Set the Attribute's modDate
  KjNode* attrModDateP = kjLookup(dbAttrP, "modDate");
  if (attrModDateP != NULL)
    attrModDateP->value.f = orionldState.requestTime;

  bson_t attr;
  mongocKjTreeToBson(dbAttrP, &attr);
  bson_append_document(&set, attrPath, -1, &attr);
  bson_destroy(&attr);

  // Update the Entity's modDate
  bson_append_double(&set, "modDate", 7, orionldState.requestTime);

  bson_append_document(&request, "$set", 4, &set);
  bson_destroy(&set);

  MONGOC_WLOG("Adding Attributes", orionldState.tenantP->mongoDbName, "entities", &selector, &request, LmtMongoc);
  bool dbResult = mongoc_collection_update_one(orionldState.mongoc.entitiesP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);
  if (dbResult == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    *detailP = errP->message;
    LM_E(("mongoc error updating entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
  }

  bson_destroy(&request);
  bson_destroy(&reply);

  return dbResult;
}

