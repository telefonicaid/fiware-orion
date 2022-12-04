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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjArray, ...
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/attributeUpdated.h"                     // attributeUpdated
#include "orionld/common/attributeNotUpdated.h"                  // attributeNotUpdated
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_OBJECT, PCHECK_EMPTY_OBJECT, ...
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/legacyDriver/legacyPostEntity.h"               // legacyPostEntity
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // dbModelFromApiAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocAttributesAdd.h"                  // mongocAttributesAdd
#include "orionld/notifications/alteration.h"                    // alteration
#include "orionld/serviceRoutines/orionldPostEntity.h"           // Own Interface



// -----------------------------------------------------------------------------
//
// dbEntityTypeExtract -
//
KjNode* dbEntityTypeExtract(KjNode* dbEntityP)
{
  KjNode* _idP = kjLookup(dbEntityP, "_id");

  if (_idP != NULL)
    return kjLookup(_idP, "type");

  return NULL;
}



// ----------------------------------------------------------------------------
//
// attributeToDbArray -
//
static bool attributeToDbArray(KjNode* dbAttrArray, KjNode* apiAttributeP, KjNode* dbAttrsP, KjNode* attrsAddedV, bool* ignoreP)
{
  // MUST CLONE as dbModelFromApiAttribute is DESTRUCTIVE + Need copy for dbAttrArray
  KjNode* dbAttributeP = kjClone(orionldState.kjsonP, apiAttributeP);

  if (dbModelFromApiAttribute(dbAttributeP, dbAttrsP, attrsAddedV, NULL, ignoreP, true) == true)
  {
    // Change name from .names to mdNames
    KjNode* mdNamesP = kjLookup(dbAttributeP, ".names");
    if (mdNamesP != NULL)
      mdNamesP->name = (char*) "mdNames";

    kjChildAdd(dbAttrArray, dbAttributeP);
    return true;
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// dbAttrsMerge -
//
static void dbAttrsMerge(KjNode* dbAttrsP, KjNode* dbAttrsUpdate, bool replace)
{
  for (KjNode* dbAttrP = dbAttrsUpdate->value.firstChildP; dbAttrP != NULL; dbAttrP = dbAttrP->next)
  {
    if (replace)
    {
      KjNode* toRemoveP = kjLookup(dbAttrsP, dbAttrP->name);
      if (toRemoveP != NULL)
        kjChildRemove(dbAttrsP, toRemoveP);
    }

    kjChildRemove(dbAttrsUpdate, dbAttrP);
    kjChildAdd(dbAttrsP, dbAttrP);
  }
}



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
bool orionldPostEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyPostEntity();

  char* entityId                  = orionldState.wildcard[0];
  char* entityType                = orionldState.uriParams.type;
  bool  ignoreExistingAttributes  = (orionldState.uriParamOptions.noOverwrite == true)? true : false;

  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "Entity Fragment", 400);
  PCHECK_OBJECT_EMPTY(orionldState.requestTree, 0, NULL, "Entity Fragment", 400);
  PCHECK_URI(entityId, true, 0, NULL, "Entity::id in URL", 400);

  //
  // Get the entity from the database
  //
  KjNode* dbEntityP = mongocEntityLookup(entityId, entityType, NULL, NULL);
  if (dbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  // Keep untouched initial state of the entity in the database - for alterations (to check for false updates)
  KjNode* initialDbEntityP = NULL;  // kjClone(orionldState.kjsonP, dbEntityP);
  KjNode* dbAttrsP         = kjLookup(dbEntityP, "attrs");

  //
  // Check the Entity, expand averything and transform it into Normalized form
  //
  // Actually, no, let's use pCheckAttribute on every individual attribute instead
  //
  // if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
  //   return false;

  //
  // We need the Entity Type for ALTERATIONS - match subscriptions
  // For that we need to go to the database..
  // While the Entity Type is optional in the payload body, we can't trust that - the truth is in the DB
  //
  // If entity type is present in the payload body, it must be a String and identical to the entity type in the database
  //
  KjNode* dbTypeNodeP = dbEntityTypeExtract(dbEntityP);
  entityType = dbTypeNodeP->value.s;

  //
  // If the Entity Type is present in the payload body ... need to make sure it's a match
  //
  if (orionldState.payloadTypeNode != NULL)
  {
    if (strcmp(orionldState.payloadTypeNode->value.s, entityType) != 0)
    {
      orionldError(OrionldBadRequestData, "Mismatching Entity::type in payload body", "Does not coincide with the Entity::type in the database", 400);
      return false;
    }
  }

  //
  // Loop over the incoming payload tree
  // Foreach attribute
  // - lookup in DB
  // - if present in DB:
  //   - if IGNORE:
  //     - Add to 'notUpdatedArray' (steal from legacyPostEntity)
  //     - REMOVE from orionldState.requestTree (for TRoE and ALTERATION)
  //   - if REPLACE:
  //     - add to dbNewAttrV
  //     - Add to 'updatedArray' (steal from legacyPostEntity)
  // - if not present in DB
  //   - add to dbNewAttrV
  //
  // - Write to DB - just a bunch of $set in attrs (and $push? to attrNames)
  // - Merge dbEntityP with dbNewAttrV => finalDbEntityP that is needed for ALTERATIONS
  //
  KjNode* updatedP        = kjArray(orionldState.kjsonP, "updated");
  KjNode* notUpdatedP     = kjArray(orionldState.kjsonP, "notUpdated");
  KjNode* newDbAttrNamesV = kjArray(orionldState.kjsonP, NULL);
  KjNode* dbAttrsUpdate   = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrP           = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  while (attrP)
  {
    OrionldContextItem*  contextItemP = NULL;
    char*                shortName    = attrP->name;

    next = attrP->next;

    //
    // Is the attribute valid?
    //
    if (pCheckAttribute(attrP, true, NoAttributeType, false, contextItemP) == false)
    {
      kjChildRemove(orionldState.requestTree, attrP);
      attributeNotUpdated(notUpdatedP, shortName, orionldState.pd.title, orionldState.pd.detail);
      attrP = next;

      continue;
    }

    // pCheckAttribute EXPANDS the attribute name - now we can check whether it already existed
    char eqName[512];
    strncpy(eqName, attrP->name, sizeof(eqName) - 1);
    dotForEq(eqName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqName);

    if (dbAttrP != NULL)
    {
      if (ignoreExistingAttributes == true)
      {
        kjChildRemove(orionldState.requestTree, attrP);
        attributeNotUpdated(notUpdatedP, shortName, "attribute already exists", "overwrite is not allowed");
      }
      else
      {
        bool ignore = false;
        if (attributeToDbArray(dbAttrsUpdate, attrP, dbAttrsP, newDbAttrNamesV, &ignore) == false)
        {
          attributeNotUpdated(notUpdatedP, shortName, orionldState.pd.title, orionldState.pd.detail);
          kjChildRemove(orionldState.requestTree, attrP);
        }
        else
          attributeUpdated(updatedP, shortName);
      }
    }
    else
    {
      bool ignore = false;
      if (attributeToDbArray(dbAttrsUpdate, attrP, dbAttrsP, newDbAttrNamesV, &ignore) == false)
      {
        attributeNotUpdated(notUpdatedP, shortName, orionldState.pd.title, orionldState.pd.detail);
        kjChildRemove(orionldState.requestTree, attrP);
      }
      else
        attributeUpdated(updatedP, shortName);
    }

    attrP = next;
  }

  KjNode* treeForTroe = NULL;
  if (dbAttrsUpdate->value.firstChildP != NULL)
  {
    if (mongocAttributesAdd(entityId, newDbAttrNamesV, dbAttrsUpdate, false) == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocAttributesAdd failed", 500);
      return false;
    }

    // WARNING: dbAttrsMerge DESTROYS dbEntityP - the initial state of the entity ...
    // Seems like it also destroys the incoming requestTree, so, need to clone for TRoE
    // Would be nice to have it NOT destroying the incoming tree (modified by pCheckAttribute)
    //
    if (troe)
      treeForTroe = kjClone(orionldState.kjsonP, orionldState.requestTree);

    dbAttrsMerge(dbAttrsP, dbAttrsUpdate, orionldState.uriParamOptions.noOverwrite == false);

    OrionldProblemDetails  pd;
    KjNode*                finalApiEntityP = dbModelToApiEntity2(dbEntityP, false, RF_NORMALIZED, orionldState.uriParams.lang, false, &pd);

    alteration(entityId, entityType, finalApiEntityP, orionldState.requestTree, initialDbEntityP);
  }

  if (notUpdatedP->value.firstChildP == NULL)  // No errors
  {
    orionldState.httpStatusCode = 204;
    orionldState.responseTree   = NULL;
  }
  else
  {
    KjNode* responseTree = kjObject(orionldState.kjsonP, NULL);
    kjChildAdd(responseTree, notUpdatedP);
    kjChildAdd(responseTree, updatedP);

    orionldState.httpStatusCode = 207;
    orionldState.responseTree   = responseTree;
  }

  if (troe)
    orionldState.requestTree = treeForTroe;

  // The orionldState.requestTree is OK for TRoE - as ignored attributes have been removed
  return true;
}
