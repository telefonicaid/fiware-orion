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
* Author: Larysse Savanna
*/
#include "mongo/client/dbclient.h"                                    // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                             // KjNode
#include "kjson/kjRender.h"                                           // kjRender - TMP
#include "kjson/kjBuilder.h"                                          // kjArray, ...
}

#include "logMsg/logMsg.h"                                            // LM_*
#include "logMsg/traceLevels.h"                                       // Lmt*

#include "mongoBackend/MongoGlobal.h"                                 // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/orionldState.h"                              // orionldState, dbName, mongoEntitiesCollectionP
#include "orionld/db/dbCollectionPathGet.h"                           // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                               // dbDataToKjTree, dbDataFromKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesDelete.h"      // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntitiesDelete -
//
bool mongoCppLegacyEntitiesDelete(KjNode* entityIdsArray)
{
  char collectionPath[256];

  if (dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities") == -1)
  {
    LM_E(("Internal Error (dbCollectionPathGet returned -1)"));
    return false;
  }

  mongo::DBClientBase*         connectionP  = getMongoConnection();
  mongo::BulkOperationBuilder  bulk         = connectionP->initializeUnorderedBulkOp(collectionPath);
  const mongo::WriteConcern    writeConcern;
  mongo::WriteResult           writeResults;

  for (KjNode* idNodeP = entityIdsArray->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    mongo::BSONObjBuilder filterObj;

    filterObj.append("_id.id", idNodeP->value.s);
    bulk.find(filterObj.obj()).remove();
  }

  bulk.execute(&writeConcern, &writeResults);
  releaseMongoConnection(connectionP);

  return true;
}
