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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityTypesFromRegistrationsGet.h"  // Own interface



// -----------------------------------------------------------------------------
//
// typeExtract -
//
void typeExtract(KjNode* regArray, KjNode* typeArray)
{
  for (KjNode* arrItemP = regArray->value.firstChildP; arrItemP != NULL; arrItemP = arrItemP->next)
  {
    KjNode* contextRegistrationV = kjLookup(arrItemP, "contextRegistration");

    if (contextRegistrationV == NULL)
    {
      LM_W(("No contextRegistration in tree ..."));
      continue;
    }

    for (KjNode* crNodeP = contextRegistrationV->value.firstChildP; crNodeP != NULL; crNodeP = crNodeP->next)
    {
      KjNode* eNodeV = kjLookup(crNodeP, "entities");

      for (KjNode* entityP = eNodeV->value.firstChildP; entityP != NULL; entityP = entityP->next)
      {
        KjNode* typeP = kjLookup(entityP, "type");

        if (typeP != NULL)
        {
          kjChildAdd(typeArray, typeP);  // OK to break tree, as entityP is one level up and its next pointer is still intact
          typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
        }
      }
    }
  }
}



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntityTypesFromRegistrationsGet -
//
// With NGSIv2 database model, a registration loks like this:
//
// {
//   "_id" : "urn:ngsi-ld:ContextSourceRegistration:csr1a341",
//   "description" : "description of reg 1",
//   "name" : "reg_csr1a341",
//   "expiration" : NumberLong(1861869600),
//   "servicePath" : "/",
//   "contextRegistration" : [
//     {
//       "entities" : [
//         {
//           "id" : "urn:ngsi-ld:Vehicle:A456",
//           "type" : "https://uri.etsi.org/ngsi-ld/default-context/Vehicle"
//         }
//       ],
//       "attrs" : [
//         {
//           "name" : "https://uri.etsi.org/ngsi-ld/default-context/brandName",
//           "type" : "Property",
//           "isDomain" : "false"
//         },
//         {
//           "name" : "https://uri.etsi.org/ngsi-ld/default-context/speed",
//           "type" : "Property",
//           "isDomain" : "false"
//         },
//         {
//           "name" : "https://uri.etsi.org/ngsi-ld/default-context/isParked",
//           "type" : "Relationship",
//           "isDomain" : "false"
//         }
//       ],
//       "providingApplication" : "http://my.csource.org:1026"
//     }
//  ],
//  ...
//
//
KjNode* mongoCppLegacyEntityTypesFromRegistrationsGet(void)
{
  char collectionPath[256];

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "registrations");

  mongo::BSONObjBuilder  fields;
  mongo::BSONObjBuilder  filter;
  mongo::BSONObjBuilder  notEmpty;

  fields.append("contextRegistration.entities",     1);  // Entity Type is inside the 'contextRegistration.entities' field ...
  fields.append("_id",     0);
  notEmpty.append("$ne", "");
  filter.append("contextRegistration.entities.type", notEmpty.obj());

  mongo::BSONObj                        fieldsToReturn = fields.obj();
  mongo::DBClientBase*                  connectionP    = getMongoConnection();
  mongo::Query                          query(filter.obj());
  std::auto_ptr<mongo::DBClientCursor>  cursorP        = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);

  KjNode*  regArray = NULL;
  KjNode*  typeArray  = NULL;

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;
    KjNode*         kjTree = dbDataToKjTree(&bsonObj, &title, &details);

    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
    else
    {
      if (regArray == NULL)
        regArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(regArray, kjTree);
    }
  }

  if (regArray != NULL)
  {
    typeArray = kjArray(orionldState.kjsonP, NULL);
    typeExtract(regArray, typeArray);
  }

  releaseMongoConnection(connectionP);
  return typeArray;
}
