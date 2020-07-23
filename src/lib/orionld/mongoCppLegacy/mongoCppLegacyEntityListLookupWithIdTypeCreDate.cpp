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
* Author: Larysse Savanna, Ken Zangelin
*/
#include <string>

#include "mongo/client/dbclient.h"                                       // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                                // KjNode
#include "kjson/kjBuilder.h"                                             // kjArray, ...
}

#include "logMsg/logMsg.h"                                               // LM_*
#include "logMsg/traceLevels.h"                                          // Lmt*

#include "mongoBackend/MongoGlobal.h"                                    // getMongoConnection, releaseMongoConnection, ...
#include "mongoBackend/safeMongo.h"                                      // getStringFieldF, ...
#include "orionld/common/orionldState.h"                                 // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbCollectionPathGet.h"                              // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                                  // dbDataToKjTree, dbDataFromKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyDbNumberFieldGet.h"       // mongoCppLegacyDbNumberFieldGet
#include "orionld/mongoCppLegacy/mongoCppLegacyDbStringFieldGet.h"       // mongoCppLegacyDbStringFieldGet
#include "orionld/mongoCppLegacy/mongoCppLegacyDbObjectFieldGet.h"       // mongoCppLegacyDbObjectFieldGet
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityListLookupWithIdTypeCreDate.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityListLookupWithIdTypeCreDate -
//
// This function extracts (from mongo) the entities whose ID are in the vector 'entityIdsArray'.
// Instead of returning the complete information of the entities, only three fields are returned per entity, namely:
//   * Entity ID
//   * Entity Type
//   * Entity Creation Date
//
KjNode* mongoCppLegacyEntityListLookupWithIdTypeCreDate(KjNode* entityIdsArray)
{
  char collectionPath[256];

  if (dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities") == -1)
  {
    LM_E(("Internal Error (dbCollectionPathGet returned -1)"));
    return NULL;
  }

  // Build the filter for the query
  mongo::BSONObjBuilder    filter;
  mongo::BSONObjBuilder    inObj;
  mongo::BSONArrayBuilder  idList;
  mongo::BSONObjBuilder    fields;

  for (KjNode* idNodeP = entityIdsArray->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    idList.append(idNodeP->value.s);
  }

  inObj.append("$in", idList.arr());
  filter.append("_id.id", inObj.obj());

  //
  // Specify the fields to return
  //
  fields.append("_id",     1);  // "id" and "type" are inside "_id"
  fields.append("creDate", 1);

  mongo::BSONObj                        fieldsToReturn = fields.obj();
  mongo::Query                          query(filter.obj());
  mongo::DBClientBase*                  connectionP    = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP        = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);


  KjNode*  entitiesArray = NULL;
  int      entities      = 0;

  while (moreSafe(cursorP))
  {
    mongo::BSONObj bsonObj;
    std::string    errorString;

    if (!nextSafeOrErrorF(cursorP, &bsonObj, &errorString))
    {
      LM_E(("Internal Error (unable to extract entity from database: %s)", errorString.c_str()));
      continue;
    }

    mongo::BSONObj  idField;
    if (mongoCppLegacyDbObjectFieldGet(&bsonObj, "_id", &idField) == false)
    {
      LM_E(("Internal Error (unable to extract the field '_id' from an entity"));
      continue;
    }

    char* idString = mongoCppLegacyDbStringFieldGet(&idField, "id");
    if (idString == NULL)
    {
      LM_E(("Internal Error (unable to extract the field '_id.id' from an entity"));
      continue;
    }

    char* typeString = mongoCppLegacyDbStringFieldGet(&idField, "type");
    if (typeString == NULL)
    {
      LM_E(("Internal Error (unable to extract the field '_id.type' from the entity '%s'", idString));
      continue;
    }

    double creDate;
    if (mongoCppLegacyDbNumberFieldGet(&bsonObj, "creDate", &creDate) == false)
    {
      LM_E(("Internal Error (unable to extract the field 'creDate' from the entity '%s'", idString));
      continue;
    }

    KjNode* entityTree   = kjObject(orionldState.kjsonP,  NULL);
    KjNode* idNodeP      = kjString(orionldState.kjsonP,  "id",      idString);
    KjNode* typeNodeP    = kjString(orionldState.kjsonP,  "type",    typeString);
    KjNode* creDateNodeP = kjFloat(orionldState.kjsonP,   "creDate", creDate);

    kjChildAdd(entityTree, idNodeP);
    kjChildAdd(entityTree, typeNodeP);
    kjChildAdd(entityTree, creDateNodeP);


    // Create the entity array if it doesn't already exist
    if (entitiesArray == NULL)
      entitiesArray = kjArray(orionldState.kjsonP, NULL);

    // Add the Entity to the entity array
    kjChildAdd(entitiesArray, entityTree);

    // A limit of 100 entities has been established.
    ++entities;
    if (entities >= 200)
    {
      LM_W(("Too many entities - breaking loop at 200"));
      break;
    }
  }

  releaseMongoConnection(connectionP);

  return entitiesArray;
}
