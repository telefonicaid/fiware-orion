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
#include <string.h>                                               // strcmp, ...
#include <mongoc/mongoc.h>                                        // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjLookup.h"                                       // kjLookup
}

#include "logMsg/logMsg.h"                                        // LM_*

#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/tenantList.h"                            // tenantList
#include "orionld/db/dbGeoIndexLookup.h"                          // dbGeoIndexLookup
#include "orionld/mongoc/mongocConnectionGet.h"                   // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeFromBson.h"                  // mongocKjTreeFromBson
#include "orionld/mongoc/mongocGeoIndexCreate.h"                  // mongocGeoIndexCreate
#include "orionld/mongoc/mongocGeoIndexInit.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// mongocGeoIndexInit -
//
bool mongocGeoIndexInit(void)
{
  mongoc_cursor_t*  mongoCursorP;
  const bson_t*     mongoDocP;

  //
  // DB Connection
  //
  mongocConnectionGet(NULL, DbNone);

  //
  // Loop over all tenants
  //
  OrionldTenant* tenantP = &tenant0;  // tenant0->next == tenantList :)
  tenant0.next = tenantList;          // Better safe than sorry!

  tenantP = &tenant0;
  while (tenantP != NULL)
  {
    mongoc_collection_t* mCollectionP;

    //
    // Get handle to collection
    //
    mCollectionP = mongoc_client_get_collection(orionldState.mongoc.client, tenantP->mongoDbName, "entities");
    if (mCollectionP == NULL)
      LM_X(1, ("mongoc_client_get_collection failed for 'entities' collection on tenant '%s'", tenantP->mongoDbName));

    // Aggregation pipeline

    // {
    //   "pipeline":
    //   [
    //      {
    //          $project: {
    //              attributeKeys: {
    //                  $filter: {
    //                      input: { $objectToArray: "$attrs" },
    //                      as: "item",
    //                      cond: { $eq: ["$$item.v.type", "GeoProperty"] }
    //                  }
    //              }
    //          }
    //      },
    //      {
    //          $unwind: "$attributeKeys"
    //      },
    //      {
    //          $group: {
    //              _id: "$attributeKeys.k"
    //          }
    //      }
    //   ]
    // }

    bson_t* pipeline = bson_new();
    bson_t* project;
    bson_t* unwind;
    bson_t* group;
    bson_t* objectArray = bson_new();

    project = BCON_NEW("$project", "{",
                       "attributeKeys", "{",
                       "$filter", "{",
                       "input", "{", "$objectToArray", BCON_UTF8("$attrs"), "}",
                       "as", BCON_UTF8("item"),
                       "cond", "{", "$eq", "[", BCON_UTF8("$$item.v.type"), BCON_UTF8("GeoProperty"), "]", "}",
                       "}",
                       "}",
                       "}");

    unwind = BCON_NEW("$unwind", BCON_UTF8("$attributeKeys"));
    group  = BCON_NEW("$group", "{", "_id", BCON_UTF8("$attributeKeys.k"), "}");

    bson_append_document(objectArray, "0", 1, project);
    bson_append_document(objectArray, "1", 1, unwind);
    bson_append_document(objectArray, "2", 1, group);

    // Append the array to the pipeline
    bson_append_array(pipeline, "pipeline", 8, objectArray);

    if (lmTraceIsSet(LmtMongoc))
    {
      char* str = bson_as_relaxed_extended_json(pipeline, NULL);
      LM_T(LmtMongoc, ("%s", str));
      bson_free(str);
    }

    //
    // Run the query
    //
    mongoCursorP = mongoc_collection_aggregate(
        mCollectionP,
        MONGOC_QUERY_NONE,
        pipeline,
        NULL,     // Additional options, can be NULL for default options
        NULL);    // Result (pass NULL if you don't need it)


    if (mongoCursorP == NULL)
    {
      LM_E(("Internal Error (mongoc_collection_aggregate ERROR)"));
      mongoc_collection_destroy(mCollectionP);
      mongoc_cursor_destroy(mongoCursorP);
      return NULL;
    }

    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      char*   title;
      char*   detail;
      KjNode* _idObjectP;
      KjNode* _idNodeP;

      _idObjectP = mongocKjTreeFromBson(mongoDocP, &title, &detail);
      _idNodeP   = (_idObjectP == NULL)? NULL : kjLookup(_idObjectP, "_id");

      if (_idNodeP == NULL)
      {
        LM_W(("_idNodeP is NULL"));
        continue;
      }

      char* geoPropertyName = _idNodeP->value.s;

      LM_T(LmtMongoc, ("Found geoProperty: '%s'", geoPropertyName));

      if (dbGeoIndexLookup(tenantP->tenant, geoPropertyName) == NULL)
      {
        mongocGeoIndexCreate(tenantP, geoPropertyName);
        LM_T(LmtMongoc, ("Creating index for property '%s'", geoPropertyName));
      }
    }

    mongoc_cursor_destroy(mongoCursorP);
    mongoc_collection_destroy(mCollectionP);

    bson_destroy(objectArray);
    bson_destroy(group);
    bson_destroy(unwind);
    bson_destroy(project);
    bson_destroy(pipeline);

    tenantP = tenantP->next;
  }

  return true;
}
