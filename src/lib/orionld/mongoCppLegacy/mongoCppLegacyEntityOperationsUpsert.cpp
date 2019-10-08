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
* Author: Gabriel Quaresma
*/
#include "mongo/client/dbclient.h"                                         // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                                  // KjNode
#include "kjson/kjRender.h"                                                // kjRender - TMP
#include "kjson/kjBuilder.h"                                               // kjArray, ...
}

#include "logMsg/logMsg.h"                                                 // LM_*
#include "logMsg/traceLevels.h"                                            // Lmt*

#include <vector>
#include "mongoBackend/MongoGlobal.h"                                      // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/orionldState.h"                                   // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbCollectionPathGet.h"                                // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                                    // dbDataToKjTree, dbDataFromKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityOperationsUpsert.h"   // Own interface


// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityOperationsUpsert -
//
bool mongoCppLegacyEntityOperationsUpsert(KjNode* entitiesArray)
{
  char collectionPath[256];
  
  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");
  LM_TMP(("Collection Path: %s", collectionPath));

  mongo::DBClientBase*  connectionP = getMongoConnection();
  mongo::BulkOperationBuilder bulk  = connectionP->initializeUnorderedBulkOp(collectionPath);
  const mongo::WriteConcern wc;
  mongo::WriteResult writeResults;

#if 0
  int ix = 0;
  for (KjNode* entityNodeP = entitiesArray->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
  {
    // KjNode* fieldNodeP = entityNodeP->value.firstChildP;
    // if (strcmp(fieldNodeP->name, "id") == 0)
    // {
    //   LM_TMP(("ID ENTITY '%s'", fieldNodeP->value.s));
    //   mongo::BSONObjBuilder filterObj;
    //   filterObj.append("_id.id", fieldNodeP->value.s);
    //   bulk.insert(filterObj.obj());
    // }
    ix++;
  }
#endif

  bulk.execute(&wc, &writeResults);
  releaseMongoConnection(connectionP);

  return true;
}
