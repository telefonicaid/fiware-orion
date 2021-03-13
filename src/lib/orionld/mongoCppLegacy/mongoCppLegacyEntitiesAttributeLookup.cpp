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
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesAttributeLookup.h"   // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntitiesAttributeLookup -
//
KjNode* mongoCppLegacyEntitiesAttributeLookup(char** entityArray, int entitiesInArray, const char* attributeName)
{
  char    collectionPath[256];
  KjNode* kjTree = kjArray(orionldState.kjsonP, NULL);

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate filter - Entity ID and attrs::attributeName
  //
  mongo::BSONObjBuilder  filter;

  // 'in' array for entity ids
  mongo::BSONArrayBuilder idArray;

  for (int ix = 0; ix < entitiesInArray; ix++)
  {
    LM_TMP(("GEO: Adding entity id '%s' to the $in array", entityArray[ix]));
    idArray.append(entityArray[ix]);
  }
  mongo::BSONObjBuilder in;
  in.append("$in", idArray.arr());

  filter.append("_id.id", in.obj());


  // attrs.attributeName: { $exists: true }
  char                   attrPath[512];
  mongo::BSONObjBuilder  existsObj;

  existsObj.append("$exists", true);

  snprintf(attrPath, sizeof(attrPath), "attrs.%s", attributeName);
  LM_TMP(("GEO: attributeName: '%s'", attrPath));

  filter.append(attrPath, existsObj.obj());

  //
  // We don't want the entire entities - only id_id and the attribute in question
  //
  mongo::BSONObjBuilder fields;

  fields.append("_id.id",  1);
  fields.append(attrPath,  1);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());
  mongo::BSONObj                        fieldsToReturn = fields.obj();

  LM_TMP(("GEO: mongo query: %s", query.toString().c_str()));
  cursorP = connectionP->query(collectionPath, query, 0, 0, &fieldsToReturn);

  //
  // Iterating over results
  //
  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    KjNode* entityP = dbDataToKjTree(&bsonObj, false, &title, &details);
    if (entityP == NULL)
      LM_E(("%s: %s", title, details));
    else
      kjChildAdd(kjTree, entityP);
  }

  releaseMongoConnection(connectionP);

  // semGive()

  //
  // Change "value" to "object" for all attributes that are "Relationship".
  // Note that the "object" field of a Relationship is stored in the database under the field "value".
  // That fact is fixed here, by renaming the "value" to "object" for attr with type == Relationship.
  // This depends on the database model and thus should be fixed in the database layer.
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

  LM_TMP(("GEO: returning entities/attribute tree at %p", kjTree));
  return  kjTree;
}
