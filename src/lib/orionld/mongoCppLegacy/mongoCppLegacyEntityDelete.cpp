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
#include "logMsg/logMsg.h"                                            // LM_*
#include "logMsg/traceLevels.h"                                       // Lmt*

#include "mongo/client/dbclient.h"                                    // MongoDB C++ Client Legacy Driver
#include "mongoBackend/MongoGlobal.h"                                 // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                           // dbCollectionPathGet
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityDelete.h"        // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityDelete -
//
bool mongoCppLegacyEntityDelete(const char* entityId)
{
  char collectionPath[256];
  bool operationStatus;

  if (dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities") == -1)
  {
    LM_E(("Internal Error (dbCollectionPathGet returned -1)"));
    return false;
  }

  //
  // Populate filter
  //
  mongo::BSONObjBuilder  filter;
  filter.append("_id.id", entityId);

  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  connectionP->remove(collectionPath, query, true);
  operationStatus = (connectionP->isFailed() == true)? false : true;

  releaseMongoConnection(connectionP);

  return operationStatus;
}