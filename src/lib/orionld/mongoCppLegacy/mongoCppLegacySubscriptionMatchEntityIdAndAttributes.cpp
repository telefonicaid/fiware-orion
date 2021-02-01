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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/orionldState.h"                         // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree

#include "orionld/mongoCppLegacy/mongoCppLegacySubscriptionMatchEntityIdAndAttributes.h"   // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacySubscriptionMatchEntityIdAndAttributes -
//
// PARAMETERS
//   * entityId             The ID of the entity as a string
//   * currentEntityTree    The entire Entity as it is in the database before being updated
//   * incomingRequestTree  The incoming request, supposed to modify the current Entity
//   * subMatchCallback     The callback function to be called for each matching subscription
//
void mongoCppLegacySubscriptionMatchEntityIdAndAttributes
(
  const char*                 entityId,
  KjNode*                     currentEntityTree,
  KjNode*                     incomingRequestTree,
  DbSubscriptionMatchCallback subMatchCallback
)
{
  char                   collectionPath[256];
  mongo::BSONObjBuilder  filter;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "csubs");

  //
  // 1. Entity ID, which in this case is FIXED
  //
  filter.append("entities.id", entityId);


  //
  // 2. Attributes
  //
  mongo::BSONArrayBuilder  attrArray;
  mongo::BSONObjBuilder    inForAttrNames;

  for (KjNode* attrNodeP = incomingRequestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    attrArray.append(attrNodeP->name);
  }

  inForAttrNames.append("$in", attrArray.arr());
  filter.append("conditions", inForAttrNames.obj());


  //
  // status - must be "active"
  //
  filter.append("status", "active");


  // "expiration" - later
  // "q" - later
  // "geometry" - later


  //
  // Perform query
  //
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  cursorP = connectionP->query(collectionPath, query);

  int niIx = 0;

  while (cursorP->more())
  {
    mongo::BSONObj            bsonObj;
    KjNode*                   subscriptionTree;
    char*                     title;
    char*                     detail;

    bsonObj = cursorP->nextSafe();

    subscriptionTree = dbDataToKjTree(&bsonObj, false, &title, &detail);
    if (subscriptionTree == NULL)
    {
      LM_E(("Internal Error (unable to create KjNode tree from mongo::BSONObj '%s')", bsonObj.toString().c_str()));
      continue;
    }

    //
    // Found a matching subscription - now the caller of this function can do whatever he/she needs to do with it
    //
    subMatchCallback(entityId, subscriptionTree, currentEntityTree, incomingRequestTree);

    //
    // A maximum of 100 notifications has been agreed (agreed in solitary by KZ :-D)
    //
    ++niIx;
    if (niIx >= 100)
      break;
  }
}
