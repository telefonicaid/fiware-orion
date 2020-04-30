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
#include "mongo/client/dbclient.h"                               // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityLookup.h"   // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityLookup -
//
KjNode* mongoCppLegacyEntityLookup(const char* entityId)
{
  char    collectionPath[256];
  KjNode* kjTree = NULL;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate 'queryBuilder' - only Entity ID for this operation
  //
  mongo::BSONObjBuilder  queryBuilder;
  queryBuilder.append("_id.id", entityId);


  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(queryBuilder.obj());

  cursorP = connectionP->query(collectionPath, query);

  //
  // FIXME: Should not be a while-loop! Only ONE entity!!
  //
  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    kjTree = dbDataToKjTree(&bsonObj, &title, &details);
    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
    break;
  }

  releaseMongoConnection(connectionP);

  // semGive()

  //
  // Change "value" to "object" for all attributes that are "Relationship".
  // Note that the "object" field of a Relationship is stored in the database under the field "value".
  // That fact is fixed here, by renaming the "value" to "object" for attr with type == Relationship.
  // This depends on the database model and thus should be fixed in the database layer.
  //
  if (kjTree != NULL)
  {
    KjNode* attrArrayP = kjLookup(kjTree, "attrs");
    if (attrArrayP != NULL)
    {
      for (KjNode* attrP = attrArrayP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        KjNode* typeP = kjLookup(attrP, "type");
        KjNode* mdsP  = kjLookup(attrP, "md");

        if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
        {
          KjNode* valueP = kjLookup(attrP, "value");

          valueP->name = (char*) "object";
        }

        if (mdsP != NULL)
        {
          for (KjNode* mdP = mdsP->value.firstChildP; mdP != NULL; mdP = mdP->next)
          {
            KjNode* typeP = kjLookup(mdP, "type");
            if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
            {
              KjNode* valueP = kjLookup(mdP, "value");
              valueP->name = (char*) "object";
            }
          }
        }
      }
    }
  }

  return kjTree;
}
