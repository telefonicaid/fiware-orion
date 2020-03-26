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
#include "mongo/client/dbclient.h"                                 // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                          // KjNode
#include "kjson/kjBuilder.h"                                       // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                         // LM_*
#include "logMsg/traceLevels.h"                                    // Lmt*

#include "mongoBackend/MongoGlobal.h"                              // getMongoConnection, releaseMongoConnection, ...

#include "orionld/db/dbCollectionPathGet.h"                        // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                            // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacySubscriptionGet.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacySubscriptionGet -
//
KjNode* mongoCppLegacySubscriptionGet(const char* subscriptionId)
{
  char     collectionPath[256];
  KjNode*  kjTree = NULL;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "csubs");

  //
  // Populate filter - only Subscription ID for this operation
  //
  mongo::BSONObjBuilder  filter;
  filter.append("_id", subscriptionId);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  cursorP = connectionP->query(collectionPath, query);

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    kjTree = dbDataToKjTree(&bsonObj, &title, &details);
    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
  }

  releaseMongoConnection(connectionP);

  // semGive()

  return kjTree;
}
