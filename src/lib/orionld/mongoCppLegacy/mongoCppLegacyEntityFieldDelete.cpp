/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, dbName, mongoEntitiesCollectionP

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityFieldDelete.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityFieldDelete -
//
bool mongoCppLegacyEntityFieldDelete(const char* entityId, const char* fieldPath)
{
  //
  // Populate filter
  //
  mongo::BSONObjBuilder  filter;
  filter.append("_id.id", entityId);


  //
  // Populate update
  //
  mongo::BSONObj update = BSON("$unset" << BSON(fieldPath << 1));

  //
  // Perform the DB action
  //
  mongo::DBClientBase*  connectionP = getMongoConnection();
  bool                  upsert      = false;
  mongo::Query          query(filter.obj());

  connectionP->update(orionldState.tenantP->entities, query, update, upsert, false);
  releaseMongoConnection(connectionP);

  return true;
}
