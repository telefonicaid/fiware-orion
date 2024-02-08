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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocAttributeDelete.h"                // Own interface



// -----------------------------------------------------------------------------
//
// mongocAttributeDelete -
//
// To delete an attribute, three (four) different places must be "touched" in an Entity:
//   * Pull 'attrName' from "attrNames"
//   * Remove 'attrs.attrNameEq'
//   * Update the 'modDate' of the entity
//   * Remove '$datasets.attrNameEq'  (if datasetId is to be supported)
//
// PARAMETERS
//   * entityId   The identifier of the Entity to be modified
//   * attrName   The real name (expanded) of the attribute to be removed
//
bool mongocAttributeDelete(const char* entityId, const char* attrName)
{
  mongocConnectionGet(orionldState.tenantP, DbEntities);

  bson_t selector;
  bson_init(&selector);
  bson_append_utf8(&selector, "_id.id", 6, entityId, -1);

  char* attrNameEq = kaStrdup(&orionldState.kalloc, attrName);
  dotForEq(attrNameEq);

  bson_t request;
  bson_t reply;
  bson_t set;    // For Entity::modDate
  bson_t unset;  // For attrs.$attrName (here we use attrNameEq)
  bson_t pull;   // For attrNames       (here we use attrName)

  bson_init(&request);
  bson_init(&reply);
  bson_init(&set);
  bson_init(&unset);
  bson_init(&pull);

  // UPDATE Entity::modDate
  bson_append_double(&set, "modDate", 7, orionldState.requestTime);

  // PULL attrNames[$attrName]
  bson_append_utf8(&pull, "attrNames", 9, attrName, -1);

  // DELETE attrs.$attrNameEq
  char path[512];
  int  pathLen;
  pathLen = snprintf(path, sizeof(path) - 1, "attrs.%s", attrNameEq);
  bson_append_utf8(&unset, path, pathLen, "", 0);

  // Add the three updates to the request
  bson_append_document(&request, "$set",   4, &set);
  bson_append_document(&request, "$unset", 6, &unset);
  bson_append_document(&request, "$pull",  5, &pull);

  // Send the request to mongo
  bool b = mongoc_collection_update_one(orionldState.mongoc.entitiesP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("mongoc error updating entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
  }

  bson_destroy(&request);
  bson_destroy(&reply);
  bson_destroy(&set);
  bson_destroy(&unset);
  bson_destroy(&pull);

  return b;
}
