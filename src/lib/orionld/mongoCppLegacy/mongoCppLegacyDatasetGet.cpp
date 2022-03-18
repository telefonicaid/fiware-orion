/*
*
* Copyright 2011 FIWARE Foundation e.V.
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

#include "orionld/common/orionldState.h"                         // orionldState

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyDatasetGet.h"     // Own interface



// ----------------------------------------------------------------------------
//
// mongoCppLegacyDatasetGet -
//
KjNode* mongoCppLegacyDatasetGet(const char* entityId, const char* attributeNameExpandedEq, const char* datasetId)
{
  KjNode* kjTree = NULL;

  //
  // Populate filter - Entity ID and @datasets::attributeNameExpandedEq::datasetId
  //
  char                   attrDataset[256];
  char                   attrDatasetId[256];
  mongo::BSONObjBuilder  match;
  mongo::BSONObjBuilder  fields;  // We only want the dataset

  snprintf(attrDataset,   sizeof(attrDataset),   "@datasets.%s",           attributeNameExpandedEq);
  snprintf(attrDatasetId, sizeof(attrDatasetId), "@datasets.%s.datasetId", attributeNameExpandedEq);

  match.append("_id.id", entityId);
  match.append(attrDatasetId, datasetId);

  fields.append(attrDataset, 1);  // Include @dataset for the attribute - ONLY
  fields.append("_id",       0);  // Exclude _id for the Entity (it's included by default)

  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(match.obj());
  mongo::BSONObj                        fieldsToReturn = fields.obj();

  cursorP = connectionP->query(orionldState.tenantP->entities, query, 0, 0, &fieldsToReturn);

  if (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    if (bsonObj.isEmpty())
      kjTree = NULL;
    else if (bsonObj.firstElementType() == mongo::Object)  // Seems to be an array
    {
      for (mongo::BSONObj::iterator iter = bsonObj.begin(); iter.more();)
      {
        mongo::BSONElement be = iter.next();
        mongo::BSONObj bob = be.Obj();
        kjTree = dbDataToKjTree(&bob, false, &title, &details);
      }
    }
    else
    {
      kjTree = dbDataToKjTree(&bsonObj, false, &title, &details);
      if (kjTree == NULL)
        LM_E(("%s: %s", title, details));
    }
  }

  releaseMongoConnection(connectionP);

#if 0
  // Not sure this is needed inside @datasets - so, #if 0, for now ...

  //
  // Change "value" to "object" for all attributes that are "Relationship".
  // Note that the "object" field of a Relationship is stored in the database under the field "value".
  // That fact is fixed here, by renaming the "value" to "object" for attr with type == Relationship.
  // This depends on the database model and thus should be fixed in the database layer.
  //
  if (kjTree != NULL)
  {
    KjNode* typeP = kjLookup(kjTree, "type");

    if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
    {
      KjNode* valueP = kjLookup(attrP, "value");
      valueP->name = (char*) "object";
    }
  }
#endif

  //
  // Post processing
  //
  // The query five us an object looking EITHER like this:
  // "@datasets": {
  //   "https://uri=etsi=org/ngsi-ld/default-context/P1": {
  //     "type":"Property",
  //     "value":1,
  //     "datasetId":"urn:ngsi-ld:dataset:D1",
  //     ...
  //   }
  // }
  //
  // OR, like this (P1 is an array if there are more than one datasets for the attribute):
  // "@datasets": {
  //   "https://uri=etsi=org/ngsi-ld/default-context/P1": [
  //     {
  //       "type":"Property",
  //       "value":1,
  //       "datasetId":"urn:ngsi-ld:dataset:D1",
  //       ...
  //     },
  //     {
  //       "type":"Property",
  //       "value":2,
  //       "datasetId":"urn:ngsi-ld:dataset:D2",
  //       ...
  //     }
  //   ]
  // }
  //
  // As the info on array or not is necessary "on the other side", we just return the first (and only) child
  //
  if (kjTree != NULL)
    return kjTree->value.firstChildP;

  return NULL;
}
