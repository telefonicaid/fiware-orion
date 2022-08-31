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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocAttributesAdd.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// mongocAttributesAdd -
//
// db.entities.update_one({_id.id: entityId}, {attrNames: }
//
bool mongocAttributesAdd
(
  const char*  entityId,
  KjNode*      newDbAttrNamesV,
  KjNode*      attrsToUpdate
)
{
  mongocConnectionGet();

  if (orionldState.mongoc.entitiesP == NULL)
    orionldState.mongoc.entitiesP = mongoc_client_get_collection(orionldState.mongoc.client, orionldState.tenantP->mongoDbName, "entities");

  bson_t selector;
  bson_init(&selector);
  bson_append_utf8(&selector, "_id.id", 6, entityId, -1);

  bson_t request;
  bson_t reply;

  bson_init(&request);
  bson_init(&reply);

  //
  // Any attributes added?
  // If so, the "attrNames field needs to be added to - with the attr names in newDbAttrNamesV
  //
  if ((newDbAttrNamesV != NULL) && (newDbAttrNamesV->value.firstChildP != NULL))  // Non-empty
  {
    bson_t push;
    bson_t eachBson;
    bson_t commaArrayBson;

    bson_init(&push);
    bson_init(&eachBson);
    bson_init(&commaArrayBson);

    bson_append_document_begin(&push, "attrNames", 9,  &eachBson);
    bson_append_array_begin(&eachBson, "$each", 5, &commaArrayBson);

    for (KjNode* attrNameP = newDbAttrNamesV->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
    {
      bson_append_utf8(&commaArrayBson, "0", 1, attrNameP->value.s, -1);  // Always index "0" ... seems to work !
    }

    bson_append_array_end(&eachBson, &commaArrayBson);
    bson_append_document_end(&push, &eachBson);
    bson_append_document(&request, "$push",  5, &push);

    bson_destroy(&commaArrayBson);
    bson_destroy(&eachBson);
    bson_destroy(&push);
  }

  char   attrPath[512];
  bson_t set;

  bson_init(&set);
  strcpy(attrPath, "attrs.");

  //
  // Populating the $set array with attributes from 'attrsToUpdate'
  //
  for (KjNode* dbAttrP = attrsToUpdate->value.firstChildP; dbAttrP != NULL; dbAttrP = dbAttrP->next)
  {
    bson_t attr;
    mongocKjTreeToBson(dbAttrP, &attr);
    strncpy(&attrPath[6], dbAttrP->name, 505);
    bson_append_document(&set, attrPath, -1, &attr);
    bson_destroy(&attr);
  }

  bson_append_document(&request, "$set", 4, &set);
  bson_destroy(&set);

  bool b = mongoc_collection_update_one(orionldState.mongoc.entitiesP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("mongoc error updating entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
  }

  bson_destroy(&request);
  bson_destroy(&reply);

  return true;
}
