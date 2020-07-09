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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityAttributesDelete.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityAttributesDelete -
//
// Mongo Shell Example:
//   db.entities.update({"_id.id": "urn:ngsi-ld:entities:E1"},
//                      {"$unset": { "attrs.https://uri=etsi=org/ngsi-ld/default-context/P1": 1, "attrs.https://uri=etsi=org/ngsi-ld/default-context/P2": 1 }})
//                      {"$pull":  { "attrNames": { "$in": [ "attrs.https://uri=etsi=org/ngsi-ld/default-context/P1", "attrs.https://uri=etsi=org/ngsi-ld/default-context/P2" ] }}}
//
bool mongoCppLegacyEntityAttributesDelete(const char* entityId, char** attrNameV, int vecSize)
{
  char                     collectionPath[256];
  mongo::BSONObjBuilder    filter;
  mongo::BSONObjBuilder    command;
  mongo::BSONObjBuilder    unset;
  mongo::BSONObjBuilder    pull;
  mongo::BSONObjBuilder    pullIn;
  mongo::BSONArrayBuilder  pullInVec;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");

  //
  // Entity ID
  //
  filter.append("_id.id", entityId);

  //
  // Attributes to remove from 'attrs', using $unset
  // Due to the database model, we also need to remove the attributes from 'attrNames', using $pull
  //
  for (int ix = 0; ix < vecSize; ix++)
  {
    int   attrNameLen   = strlen(attrNameV[ix]);
    int   mongoPathLen  = 6  + attrNameLen + 1;  // 6  == strlen("attrs."),     1 == zero-termination
    char* mongoPath     = (char*) kaAlloc(&orionldState.kalloc, mongoPathLen);

    snprintf(mongoPath, mongoPathLen, "attrs.%s", attrNameV[ix]);
    unset.append(mongoPath, 1);

    eqForDot(attrNameV[ix]);
    pullInVec.append(attrNameV[ix]);
  }

  pullIn.append("$in", pullInVec.arr());
  pull.append("attrNames", pullIn.obj());
  command.append("$unset", unset.obj());
  command.append("$pull",  pull.obj());

  //
  // Updating database
  //
  // semTake()
  mongo::DBClientBase*    connectionP = getMongoConnection();
  mongo::BSONObj          commandObj  = command.obj();
  mongo::Query            query(filter.obj());

  connectionP->update(collectionPath, query, commandObj, true, false);

  releaseMongoConnection(connectionP);
  // semGive()

  return true;
}
