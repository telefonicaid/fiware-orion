/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "mongo/client/dbclient.h"                                   // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                            // KjNode
#include "kjson/kjLookup.h"                                          // kjLookup
}

#include "logMsg/logMsg.h"                                           // LM_*
#include "logMsg/traceLevels.h"                                      // Lmt*

#include "mongoBackend/MongoGlobal.h"                                // getMongoConnection, releaseMongoConnection, ...

#include "orionld/common/orionldState.h"                             // tenants, tenantV
#include "orionld/db/dbCollectionPathGet.h"                          // dbCollectionPathGetWithTenant
#include "orionld/db/dbGeoIndexLookup.h"                             // dbGeoIndexLookup
#include "orionld/mongoCppLegacy/mongoCppLegacyGeoIndexCreate.h"     // mongoCppLegacyGeoIndexCreate
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeFromBsonObj.h"  // mongoCppLegacyKjTreeFromBsonObj
#include "orionld/mongoCppLegacy/mongoCppLegacyGeoIndexInit.h"       // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyGeoIndexInit -
//
void mongoCppLegacyGeoIndexInit(void)
{
  //
  // Loop over all tenants
  // Index -1 is used for the default tenant
  //
  for (int ix = -1; ix < (int) tenants; ix++)
  {
    char  collectionPath[256];
    char* tenant = (ix == -1)? dbName : tenantV[ix];

    dbCollectionPathGetWithTenant(collectionPath, sizeof(collectionPath), tenant, "entities");

    // Foreach ENTITY (only attrs)
    mongo::BSONObjBuilder  dbFields;

    dbFields.append("attrs", 1);
    dbFields.append("_id", 0);

    mongo::BSONObjBuilder                 filter;
    mongo::Query                          query(filter.obj());
    mongo::BSONObj                        fieldsToReturn = dbFields.obj();
    mongo::DBClientBase*                  connectionP    = getMongoConnection();
    std::auto_ptr<mongo::DBClientCursor>  cursorP        = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);

    orionldState.jsonBuf = NULL;
    while (cursorP->more())
    {
      if (orionldState.jsonBuf != NULL)
      {
        // mongoCppLegacyKjTreeFromBsonObj uses orionldState.jsonBuf for its output tree
        free(orionldState.jsonBuf);
        orionldState.jsonBuf = NULL;
      }

      mongo::BSONObj  bsonObj = cursorP->nextSafe();

      char*           title;
      char*           detail;
      KjNode*         kjTree  = mongoCppLegacyKjTreeFromBsonObj(&bsonObj, &title, &detail);
      KjNode*         attrsP = kjTree->value.firstChildP;

      if (attrsP == NULL)  //  Entity without attributes ?
        continue;

      // Foreach PROPERTY
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
          if (dbGeoIndexLookup(tenant, attrP->name) == NULL)
            mongoCppLegacyGeoIndexCreate(tenant, attrP->name);
        }
      }
    }
  }
}
