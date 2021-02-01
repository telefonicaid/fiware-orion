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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongo/client/dbclient.h"                               // MongoDB C++ Client Legacy Driver
#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree


// -----------------------------------------------------------------------------
//
// mongoCppLegacyRegistrationGet -
//
// If attribute is NULL: query registrations collection for:
//   db.registrations.find({ "contextRegistration.entities.id": "urn:ngsi-ld:entities:E1" })
//
// If attribute is non-NULL:
//   db.registrations.find(
//     {
//       "contextRegistration.entities.id": "urn:ngsi-ld:entities:E1",
//       $or: [
//         { "contextRegistration.attrs": { "$size": 0 } },
//         { "contextRegistration.attrs.name": "https://uri.etsi.org/ngsi-ld/default-context/A1" }
//       ]
//     }
//   )
//
// ToDo
//   o Include idPattern in the query
//
KjNode* mongoCppLegacyRegistrationGet(const char* registrationId)
{
  char    collectionPath[256];

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "registrations");

  //
  // Populate filter - only Registration ID for this operation
  //
  mongo::BSONObjBuilder  filter;
  filter.append("_id", registrationId);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  cursorP = connectionP->query(collectionPath, query);

  mongo::BSONObj  bsonObj = cursorP->nextSafe();

  char*    title;
  char*    details;
  KjNode*  registrationP;

  registrationP = dbDataToKjTree(&bsonObj, false, &title, &details);

  if (registrationP == NULL)
    LM_E(("%s: %s", title, details));

  releaseMongoConnection(connectionP);

  // semGive()

  return registrationP;
}
