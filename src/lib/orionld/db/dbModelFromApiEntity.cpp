/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjArray, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/kjTree/kjArrayAdd.h"                           // kjArrayAdd
#include "orionld/db/dbModelFromApiAttribute.h"                  // dbModelFromApiAttribute
#include "orionld/db/dbModelFromApiEntity.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiEntity - modify the request tree to match the db model
//
// Modifications:
//
//   * Entity Level (dbModelEntity)
//     * "id" can't be modified    (make sure it's removed and partial error reported by pCheckEntity)
//     * "type" can't be modified  (make sure it's removed and partial error reported by pCheckEntity)
//     * "scope" doesn't exist yet (make sure it's removed and partial error reported by pCheckEntity)
//     * "modDate" is set with orionldState.requestTime
//     * "attrs" member is created and added
//     * It's an attribute - move to "attrs" and call "attributeToDbModel" on it.
//     * if Array - datasetId - error for now (partial 501)
//     * if != Object - error (as pCheckAttribute makes sure the request tree is normalized)
//     * move the attribute there + call "Level 1 Function" (orionldDbModelAttribute)
//
//   * Attribute Level (dbModelAttribute)
//     * If Array, recursive call for each member (set the name to the sttribute name)
//     * "datasetId" present - call orionldDbModelAttributeDatasetId
//     * "type" can't be modified
//     * "value"/"object"/"languageMap" changes name to "value" and RHS stays as is
//     * "observedAt" is made an Object with single member "value"
//     * "unitCode" is made an Object with single member "value"
//     * "md" is created and added
//
//   * Sub-Attribute Level (dbModelSubAttribute)
//
bool dbModelFromApiEntity(KjNode* entityP, KjNode* dbAttrsP, KjNode* dbAttrNamesP)
{
  KjNode*      nodeP;
  const char*  mustGo[] = { "_id", "id", "@id", "type", "@type", "scope", "createdAt", "modifiedAt", "creDate", "modDate" };

  //
  // Remove any non-attribute nodes
  // PATCH-able toplevel fields (type? scope?) would need to be removed and saved. Later reintroduced into entityP
  //
  for (unsigned int ix = 0; ix < sizeof(mustGo) / sizeof(mustGo[0]); ix++)
  {
    nodeP = kjLookup(entityP, mustGo[ix]);
    if (nodeP != NULL)
      kjChildRemove(entityP, nodeP);
  }


  //
  // Create the "attrs" field and move all attributes from entityP to there ("attrs")
  // Then move "attrs" inside entityP
  //
  KjNode* attrsP = kjObject(orionldState.kjsonP, "attrs");
  attrsP->value.firstChildP = entityP->value.firstChildP;
  attrsP->lastChild         = entityP->lastChild;

  // Make "attrs" the only child (thus far) of entityP
  entityP->value.firstChildP  = attrsP;
  entityP->lastChild          = attrsP;

  // Rename "attrNames" to ".names" and add it to the entity
  if (dbAttrNamesP == NULL)
  {
    // No 'attrNames' field in DB Entity - must be the entity had zero attrs before this
    dbAttrNamesP = kjArray(orionldState.kjsonP, ".names");
  }
  else
    dbAttrNamesP->name = (char*) ".names";

  // Now we can add "attrNames"
  kjChildAdd(entityP, dbAttrNamesP);

  // Adding members necessary for the DB Model
  kjTimestampAdd(entityP, "modDate");
  KjNode* attrAddedV   = kjArrayAdd(entityP, ".added");
  KjNode* attrRemovedV = kjArrayAdd(entityP, ".removed");  // Not really necessary ... the RHS == null already tells the next layer

  //
  // Loop over the "attrs" member of entityP and call dbModelFromApiAttribute for every attribute
  //
  for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    dbModelFromApiAttribute(attrP, dbAttrsP, attrAddedV, attrRemovedV);
  }

  return true;
}
