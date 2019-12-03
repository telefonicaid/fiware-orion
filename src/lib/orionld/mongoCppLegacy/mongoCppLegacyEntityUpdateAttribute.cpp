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
* Author: Gabriel Quaresma
*/
#include "mongo/client/dbclient.h"                               // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                                     // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/orionldState.h"                                  // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbCollectionPathGet.h"                               // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                                   // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityUpdateAttribute.h"   // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityUpdateAttribute -
//
bool mongoCppLegacyEntityUpdateAttribute(const char* entityId, KjNode* attributeNode)
{

  char           collectionPath[256];
  
  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate filter - only Entity ID for this operation
  //
  mongo::BSONObjBuilder  filter;
  mongo::BSONObjBuilder  updatedAttrEntity;
  mongo::BSONObjBuilder  attributeUpdate; // $set
  
  filter.append("_id.id", entityId);
  
  if (attributeNode->type == KjInt)
    attributeUpdate.append(attributeNode->name, (int) attributeNode->value.i);
  else if (attributeNode->type == KjString)
    attributeUpdate.append(attributeNode->name, attributeNode->value.s);

  updatedAttrEntity.append("$set", attributeUpdate.obj());

  mongo::BSONObj updatedEntityObj = updatedAttrEntity.obj();


  // semTake()
  bool                  upsert      = false;
  mongo::DBClientBase*  connectionP = getMongoConnection();
  mongo::Query          query(filter.obj());

  LM_TMP(("mongoCppLegacyEntityUpdateAttribute: filter: %s", query.toString().c_str()));

  connectionP->update(collectionPath, query, updatedEntityObj, upsert, false);
  releaseMongoConnection(connectionP);
  // semGive()

  return true;
}
