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
#include <unistd.h>                                              // NULL

#include "mongo/client/dbclient.h"                               // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesGet.h"    // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntitiesGet -
//
KjNode* mongoCppLegacyEntitiesGet(char** fieldV, int fields)
{
  char collectionPath[256];
  bool idPresent = false;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");

  mongo::BSONObjBuilder  dbFields;

  for (int ix = 0; ix < fields; ix++)
  {
    dbFields.append(fieldV[ix], 1);

    if (strcmp(fieldV[ix], "_id") == 0)
      idPresent = true;
  }

  if (idPresent == false)
    dbFields.append("_id", 0);

  mongo::BSONObjBuilder                 filter;
  mongo::Query                          query(filter.obj());
  mongo::BSONObj                        fieldsToReturn = dbFields.obj();
  mongo::DBClientBase*                  connectionP    = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP        = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);
  KjNode*                               entityArray    = NULL;

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;
    KjNode*         kjTree = dbDataToKjTree(&bsonObj, &title, &details);

    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
    else
    {
      if (entityArray == NULL)
        entityArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(entityArray, kjTree);
    }
  }

  releaseMongoConnection(connectionP);
  return entityArray;
}
