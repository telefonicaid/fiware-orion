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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityTypeGet.h"  // Own interface



// -----------------------------------------------------------------------------
//
// kjAttributesWithTypeExtract - convert DB entity::attrs field into list of attrs with type
//
// kjTree (incoming):
// {
//   "attrs": {
//     "P1": {
//       "type": "Property",
//       ...
//     },
//     "R1": {
//       "type": "Relationship",
//       ...
//     },
//     ...
//   }
// }
//
// entityP (outgoing):
// {
//   "P1": "Property",
//   "R1": "Relationship"
// }
//
// Also, the '=' in attribute names are to be replaced with '.' (NGSI data model details)
//
static bool kjAttributesWithTypeExtract(KjNode* kjTree, KjNode* entityP)
{
  KjNode* attrsP = kjLookup(kjTree, "attrs");

  if (attrsP == NULL)
    return false;

  if (attrsP->type != KjObject)
    return false;

  KjNode* attrP = attrsP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    KjNode* typeP = kjLookup(attrP, "type");

    if (typeP != NULL)
    {
      // Set 'attrP' to have the value of 'typeP', ad that's whay we want for 'entityP'
      attrP->value = typeP->value;
      attrP->type  = typeP->type;

      kjChildRemove(attrsP, attrP);
      kjChildAdd(entityP, attrP);

      eqForDot(attrP->name);
    }

    attrP = next;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityTypeGet -
//
// Example broker response payload body for GET /types/{typeName}:
// {
//   "id": <FQN of the type>,
//   "type": "EntityTypeInformation",
//   "typeName": <shortname>,           // Mandatory! (will try to change that - only if shortName found)
//   "entityCount": 27,                 // Number of entities with this type
//   "attributeDetails": [              // Array of Attribute - See GET /attributes?details=true
//     {
//       "id": "",
//       "type": "",
//       "attributeName": "",
//       "attributeTypes": []
//     },
//     {
//     },
//     ...
//   ]
// }
//
// Now, in order to build such a tree, the info we need from each and every matching Entity (with the specified entity type)
// is just the attributes with their types.
// We already have the type for the entity and we don't need the entity id.
// We just need the attribute name and its type (Property, Relationship, ...) of all matching entities
// [
//   {
//     "P1": "Property",
//     "P2": "Property",
//     "R1": "Relationship",
//     "R2": "Relationship"
//   },
//   {
//   },
//   ...
// ]
//
KjNode* mongoCppLegacyEntityTypeGet(OrionldProblemDetails* pdP, const char* typeLongName, int* noOfEntitiesP)
{
  //
  // Populate 'queryBuilder' - only Entity ID for this operation
  //
  mongo::BSONObjBuilder  queryBuilder;
  queryBuilder.append("_id.type", typeLongName);

  //
  // We only want the "attrs" field back
  //
  mongo::BSONObjBuilder  dbFields;

  dbFields.append("_id",   0);
  dbFields.append("attrs", 1);

  mongo::Query                          query(queryBuilder.obj());
  mongo::DBClientBase*                  connectionP    = getMongoConnection();
  mongo::BSONObj                        fieldsToReturn = dbFields.obj();
  std::auto_ptr<mongo::DBClientCursor>  cursorP        = connectionP->query(orionldState.tenantP->entities, query, 0, 0, &fieldsToReturn);
  KjNode*                               outArray       = kjArray(orionldState.kjsonP, NULL);
  int                                   entities       = 0;

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;
    KjNode*         kjTree = dbDataToKjTree(&bsonObj, false, &title, &details);

    if (kjTree == NULL)
    {
      LM_E(("%s: %s", title, details));
      continue;
    }

    KjNode* entityP = kjObject(orionldState.kjsonP, NULL);
    if (kjAttributesWithTypeExtract(kjTree, entityP) == true)
    {
      ++entities;
      kjChildAdd(outArray, entityP);
    }
  }

  *noOfEntitiesP = entities;

  releaseMongoConnection(connectionP);

  return outArray;
}
