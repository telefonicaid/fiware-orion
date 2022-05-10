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

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/kjTree/kjArrayAdd.h"                           // kjArrayAdd
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // dbModelFromApiAttribute
#include "orionld/dbModel/dbModelFromApiEntity.h"                // Own interface



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
//
// The order of the items of an entity in the database (NGSIv1 database model) is:
//
//   * _id
//   * attrNames
//   * attrs (if present)
//   * creDate
//   * modDate
//   * lastCorrelator
//
bool dbModelFromApiEntity(KjNode* entityP, KjNode* dbAttrsP, KjNode* dbAttrNamesP, bool creation)
{
  KjNode*      nodeP;
  const char*  mustGo[] = { "_id", "id", "@id", "type", "@type", "scope", "createdAt", "modifiedAt", "creDate", "modDate" };

  //
  // Remove any non-attribute nodes from the incoming tree (entityP)
  // PATCH-able toplevel fields (type? scope?) would need to be removed and saved. Later reintroduce into entityP
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

  // Empty entityP
  entityP->value.firstChildP = NULL;
  entityP->lastChild         = NULL;


  //
  // Add to entityP:
  // - First "attrNames" (.added)
  // - Then "attrs"
  //
  KjNode* attrAddedV = kjArrayAdd(entityP, ".added");
  kjChildAdd(entityP, attrsP);

  // Not really necessary?   - RHS == null already tells the next layer ... ?
  KjNode* attrRemovedV = kjArrayAdd(entityP, ".removed");

  if (creation == true)
    kjTimestampAdd(entityP, "creDate");
  kjTimestampAdd(entityP, "modDate");


  // Rename "attrNames" to ".names" and add it to the entity
  if (dbAttrNamesP == NULL)
  {
    // No 'attrNames' field in DB Entity - must be the entity had zero attrs before this
    dbAttrNamesP = kjArray(orionldState.kjsonP, ".names");
  }
  else
    dbAttrNamesP->name = (char*) ".names";

  // Add "attrNames" to the entity
  kjChildAdd(entityP, dbAttrNamesP);

  //
  // Loop over the "attrs" member of entityP and call dbModelFromApiAttribute for every attribute
  //
  KjNode* attrP = attrsP->value.firstChildP;
  KjNode* next;
  while (attrP != NULL)
  {
    bool ignore = false;

    next = attrP->next;
    if (dbModelFromApiAttribute(attrP, dbAttrsP, attrAddedV, attrRemovedV, &ignore) == false)
    {
      if (ignore == true)
      {
        LM_TMP(("DS: Attribute '%s' to be ignored - removing it from attrsP", attrP->name));
        kjChildRemove(attrsP, attrP);
      }
      else
      {
        //
        // Not calling orionldError as a better error message is overwritten if I do.
        // Once we have "Error Stacking", orionldError should be called.
        //
        // orionldError(OrionldInternalError, "Unable to convert API Attribute into DB Model Attribute", attrP->name, 500);

        return false;
      }
    }
    else if (ignore == true)
    {
      LM_TMP(("DS: Attribute '%s' to be ignored - removing it from attrsP", attrP->name));
      kjChildRemove(attrsP, attrP);
    }

    attrP = next;
  }

  if (creation == true)
  {
    attrAddedV->name = (char*) "attrNames";
    kjChildRemove(entityP, attrRemovedV);
    kjChildRemove(entityP, dbAttrNamesP);

    //
    // "lastCorrelator" ... not used in NGSI-LD, but, for NGSIv2 backwards compatibility, it should be present in DB
    //
    KjNode* lastCorrelatorP = kjString(orionldState.kjsonP,  "lastCorrelator", "");
    kjChildAdd(entityP, lastCorrelatorP);

    // _id ...
  }

  return true;
}
