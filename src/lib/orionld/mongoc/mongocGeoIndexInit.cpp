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
  bson_t            mongoFilter;
  mongoc_cursor_t*  mongoCursorP;
  const bson_t*     mongoDocP;
  bson_t*           options = BCON_NEW("projection", "{",
                                         "attrs", BCON_BOOL(true),
                                         "_id",   BCON_BOOL(false),
                                       "}");
  //
  // Create the filter for the query - no restriction, we want all entities!
  //
  bson_init(&mongoFilter);

  //
  // DB Connection
  //
  mongocConnectionGet();

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

    //
    // Run the query
    //
    if ((mongoCursorP = mongoc_collection_find_with_opts(mCollectionP, &mongoFilter, options, NULL)) == NULL)
    {
      LM_E(("Internal Error (mongoc_collection_find_with_opts ERROR)"));
      bson_destroy(options);
      bson_destroy(&mongoFilter);
      mongoc_collection_destroy(mCollectionP);
      mongoc_cursor_destroy(mongoCursorP);
      return NULL;
    }

    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      char*   title;
      char*   detail;
      KjNode* entityNodeP;
      KjNode* attrsP;

      entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);
      if (entityNodeP == NULL)
      {
        LM_W(("mongocKjTreeFromBson failed"));
        continue;
      }

      attrsP = entityNodeP->value.firstChildP;
      if (attrsP == NULL)  //  Entity without attributes ?
      {
        LM_W(("Entity without attributes?"));
        continue;
      }

      //
      // Foreach Attribute, check if GeoProperty and if so create its geo index
      //
      for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        KjNode* typeP = kjLookup(attrP, "type");

        if (typeP == NULL)
        {
          LM_E(("Database Error (attribute without 'type' field)"));
          continue;
        }

        if (typeP->type != KjString)
        {
          LM_E(("Database Error (attribute with a 'type' field that is not a string)"));
          continue;
        }

        if (strcmp(typeP->value.s, "GeoProperty") == 0)
        {
          if (dbGeoIndexLookup(tenantP->tenant, attrP->name) == NULL)
            mongocGeoIndexCreate(tenantP, attrP->name);
        }
      }
    }

    mongoc_cursor_destroy(mongoCursorP);
    mongoc_collection_destroy(mCollectionP);
    tenantP = tenantP->next;
  }

  bson_destroy(options);
  bson_destroy(&mongoFilter);

  return true;
}
