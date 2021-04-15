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
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityAttributeWithDatasetsLookup.h"   // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityAttributeWithDatasetsLookup -
//
KjNode* mongoCppLegacyEntityAttributeWithDatasetsLookup(const char* entityId, const char* attributeName)
{
  char    collectionPath[256];
  KjNode* kjTree = NULL;

  LM_TMP(("DA: In"));
  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate filter - Entity ID and attrs::attributeName
  //
  char                   datasetInstance[512];
  mongo::BSONObjBuilder  filter;
  mongo::BSONObjBuilder  existsObj;
  mongo::BSONObjBuilder  fields;

  existsObj.append("$exists", true);

  snprintf(datasetInstance, sizeof(datasetInstance), "@datasets.%s", attributeName);
  filter.append("_id.id", entityId);

  filter.append(datasetInstance, existsObj.obj());

  fields.append("_id",       0);
  fields.append("attrs",     1);
  fields.append("@datasets", 1);

  // LM_TMP(("DA: filter: %s", filter.obj().toString().c_str()));

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
  {
    LM_TMP(("DA: Nothing matches"));
    return NULL;
  }

  char buf[2048];
  kjFastRender(orionldState.kjsonP, kjTree, buf, sizeof(buf));
  LM_TMP(("DA: got from DB: %s", buf));

  //
  // Now we have this in kjTree (for example):
  //
  // {
  //   "attrs": {},
  //   "@datasets": {
  //     "https://uri=etsi=org/ngsi-ld/default-context/A1": [
  //       {
  //         "type": "Property",
  //         "value": "A1:D3",
  //         "datasetId": "urn:ngsi-ld:dataset:D3",
  //         "createdAt": 1618392667.996676,
  //         "modifiedAt": 1618392667.996676
  //       }
  //     ]
  //   }
  // }
  //
  // What do we return?
  //   The datasets array for the attribute, including (if present) the default instance?
  //   OR: just TRUE as it exists?
  //       ('lookup' for "exists" and 'get' to get back the tree?
  //
  // What I need for "DELETE Attribute ?deleteAll=true" and TRoE is:
  // - did the default instance exist?
  // - a list of all datasetIds
  //
  // Let's just return the whole kjTree (for now) and let the caller deal with extracting stuff
  //
#if 0
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
#endif

  return  kjTree;
}
