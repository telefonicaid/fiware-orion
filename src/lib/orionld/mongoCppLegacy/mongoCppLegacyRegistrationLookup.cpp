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
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree


// -----------------------------------------------------------------------------
//
// mongoCppLegacyRegistrationLookup -
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
KjNode* mongoCppLegacyRegistrationLookup(const char* entityId, const char* attribute, int* noOfRegsP)
{
  KjNode* kjRegArray = NULL;

  if (noOfRegsP != NULL)
    *noOfRegsP = 0;

  //
  // Populate filter - on Entity ID and Attribute Name
  //
  mongo::BSONObjBuilder  filter;
  filter.append("contextRegistration.entities.id", entityId);

  if (attribute != NULL)
  {
    mongo::BSONObjBuilder   zeroSizeObject;
    mongo::BSONObjBuilder   zeroSizeArrayItem;
    mongo::BSONObjBuilder   attrNameMatchArrayItem;
    mongo::BSONArrayBuilder orArray;

    zeroSizeObject.append("$size", 0);
    zeroSizeArrayItem.append("contextRegistration.attrs", zeroSizeObject.obj());

    attrNameMatchArrayItem.append("contextRegistration.attrs.name", attribute);

    orArray.append(zeroSizeArrayItem.obj());
    orArray.append(attrNameMatchArrayItem.obj());

    filter.append("$or", orArray.arr());

    if (noOfRegsP != NULL)
      *noOfRegsP += 1;
  }

  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  cursorP = connectionP->query(orionldState.tenantP->registrations, query);

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;
    KjNode*         kjTree = dbDataToKjTree(&bsonObj, false, &title, &details);

    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
    else
    {
      if (kjRegArray == NULL)
        kjRegArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(kjRegArray, kjTree);
    }
  }

  releaseMongoConnection(connectionP);

  // semGive()

  return kjRegArray;
}
