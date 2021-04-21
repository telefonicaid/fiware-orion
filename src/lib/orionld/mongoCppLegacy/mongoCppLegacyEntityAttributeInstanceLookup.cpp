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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityAttributeInstanceLookup.h"   // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityAttributeInstanceLookup -
//
KjNode* mongoCppLegacyEntityAttributeInstanceLookup(const char* entityId, const char* attributeName, const char* datasetId)
{
  char    collectionPath[256];
  KjNode* kjTree = NULL;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate filter - Entity ID and attrs::attributeName
  //
  char                   datasetInstance[512];
  char                   datasets[512];
  mongo::BSONObjBuilder  filter;
  mongo::BSONObjBuilder  fields;

  snprintf(datasetInstance, sizeof(datasetInstance), "@datasets.%s.datasetId", attributeName);
  snprintf(datasets,        sizeof(datasets),        "@datasets.%s",           attributeName);

  filter.append("_id.id", entityId);
  filter.append(datasetInstance, datasetId);

  fields.append("_id", 0);
  fields.append(datasets, 1);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());
  mongo::BSONObj                        fieldsToReturn = fields.obj();

  cursorP = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);

  if (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    kjTree = dbDataToKjTree(&bsonObj, false, &title, &details);
    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
  }

  releaseMongoConnection(connectionP);

  if (kjTree == NULL)
    return NULL;

  // This is what we have now:
  // {
  //   "@datasets": {
  //     "https://uri=etsi=org/ngsi-ld/default-context/A1": [
  //       {
  //         "type":"X",
  //         "value":"X",
  //         "datasetId":"X",
  //         ...
  //       },
  //       {
  //         "type":"X",
  //         "value":"X",
  //         "datasetId":"X",
  //         ...
  //       }
  //     ]
  //   }
  // }

  // Lookup the correct attribute
  KjNode* instanceArray = NULL;
  for (KjNode* aP = kjTree->value.firstChildP->value.firstChildP; aP != NULL; aP = aP->next)
  {
    if (strcmp(aP->name, attributeName) == 0)
    {
      instanceArray = aP;
      break;
    }
  }

  if (instanceArray == NULL)
    return NULL;

  //
  // Lookup the object with the matching datasetId and return it
  //
  for (KjNode* aInstanceP = instanceArray->value.firstChildP; aInstanceP != NULL; aInstanceP = aInstanceP->next)
  {
    KjNode* datasetIdNodeP = kjLookup(aInstanceP, "datasetId");

    if ((datasetIdNodeP != NULL) && (datasetIdNodeP->type == KjString) && (strcmp(datasetIdNodeP->value.s, datasetId) == 0))
      return aInstanceP;
  }


  //
  // Change "value" in Relationship to "object"
  //
  if (kjTree != NULL)
  {
    KjNode* attrArrayP = kjLookup(kjTree, "attrs");
    if (attrArrayP != NULL)
    {
      for (KjNode* attrP = attrArrayP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        KjNode* typeP = kjLookup(attrP, "type");
        KjNode* mdsP  = kjLookup(attrP, "md");

        if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
        {
          KjNode* valueP = kjLookup(attrP, "value");

          valueP->name = (char*) "object";
        }

        if (mdsP != NULL)
        {
          for (KjNode* mdP = mdsP->value.firstChildP; mdP != NULL; mdP = mdP->next)
          {
            KjNode* typeP = kjLookup(mdP, "type");
            if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
            {
              KjNode* valueP = kjLookup(mdP, "value");
              valueP->name = (char*) "object";
            }
          }
        }
      }
    }
  }

  return  kjTree;
}
