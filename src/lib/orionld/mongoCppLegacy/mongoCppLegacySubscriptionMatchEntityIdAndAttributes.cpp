/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
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
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree

#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeFromBsonObj.h"                        // mongoCppLegacyKjTreeFromBsonObj
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
  char    collectionPath[256];

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "csubs");
  LM_TMP(("NFY: Subscription Collection Path: %s", collectionPath));

  //
  // 1. Entity ID, which in this case is FIXED
  //
  mongo::BSONObjBuilder  filter;

  filter.append("entities.id", entityId);
  LM_TMP(("NFY: Adding entity ID '%s' to the query filter", entityId));


  //
  // 2. Attributes
  //
  mongo::BSONArrayBuilder  attrArray;
  mongo::BSONObjBuilder    inForAttrNames;

  for (KjNode* attrNodeP = incomingRequestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    char* attrNameWithDots = kaStrdup(&orionldState.kalloc, attrNodeP->name);

    attrArray.append(attrNameWithDots);

    //
    // Now that the attribute name has been used in "attrNames", we can safely change the dots for EQ-signs
    //
    dotForEq(attrNodeP->name);
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

  // Debugging - see the query
  LM_TMP(("NFY: filter: %s", query.toString().c_str()));

  cursorP = connectionP->query(collectionPath, query);

  int niIx = 0;

  while (cursorP->more())
  {
    mongo::BSONObj            bsonObj;
    KjNode*                   subscriptionTree;
    char*                     title;
    char*                     detail;

    bsonObj = cursorP->nextSafe();

    // LM_TMP(("NFY: query result: '%s'", bsonObj.toString().c_str()));

    subscriptionTree = mongoCppLegacyKjTreeFromBsonObj(&bsonObj, &title, &detail);
    if (subscriptionTree == NULL)
    {
      LM_E(("Unable to create KjNode tree from mongo::BSONObj '%s'", bsonObj.toString().c_str()));
      continue;
    }

    subMatchCallback(entityId, subscriptionTree, currentEntityTree, incomingRequestTree);

    //
    // A maximum of 100 notifications has been agreed (agreed in solitary by KZ :-D)
    //
    ++niIx;
    if (niIx >= 100)
    {
      LM_W(("Too many notifications - breaking loop at 100"));
      break;
    }
  }
}



