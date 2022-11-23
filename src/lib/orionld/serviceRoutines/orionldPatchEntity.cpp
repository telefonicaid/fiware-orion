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
#include <string.h>                                              // strncpy

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/attributeUpdated.h"                     // attributeUpdated
#include "orionld/common/attributeNotUpdated.h"                  // attributeNotUpdated
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeType
#include "orionld/legacyDriver/legacyPatchEntity.h"              // legacyPatchEntity
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_OBJECT, PCHECK_URI, ...
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // dbModelFromApiAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocAttributesAdd.h"                  // mongocAttributesAdd
#include "orionld/notifications/alteration.h"                    // alteration
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/serviceRoutines/orionldPatchEntity.h"          // Own interface



// -----------------------------------------------------------------------------
//
// dbEntityTypeExtract - from orionldPostEntity.cpp
//
extern KjNode* dbEntityTypeExtract(KjNode* dbEntityP);



// -----------------------------------------------------------------------------
//
// attributesMerge -
//
static void attributesMerge(KjNode* finalApiEntityP, KjNode* apiAttrs)
{
  for (KjNode* apiAttrP = apiAttrs->value.firstChildP; apiAttrP != NULL; apiAttrP = apiAttrP->next)
  {
    LM(("Looking up attribute '%s'", apiAttrP->name));
    KjNode* oldAttrP = kjLookup(finalApiEntityP, apiAttrP->name);

    if (oldAttrP != NULL)
      kjChildRemove(finalApiEntityP, oldAttrP);

    kjChildAdd(finalApiEntityP, kjClone(orionldState.kjsonP, apiAttrP));
  }
}



// -----------------------------------------------------------------------------
//
// orionldPatchEntity -
//
// This operation allows modifying an existing NGSI-LD Entity by REPLACING already existing Attributes.
//
// - Attributes that do not exist give error in a 207 response
// - Attributes that do exist are REPLACED (createdAt shall remain as it was)
// - The type of the attribute cannot change.
//
// Special Attributes
//   'type':   As Orion-LD still doesn't implement multi-type, if entity type is present, it is handled with error in a 207 response.
//   'scope':  Also not implemented, but the spec somehow says to ignore it, so it's ignored ...
//
// RESPONSE
//   204    and no body if all attributes are successfully updated
//   207    and a body of UpdateResult if any errors
//
bool orionldPatchEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyPatchEntity();

  const char* entityId = orionldState.wildcard[0];

  //
  // Initial Validity Checks;
  // o Is the Entity ID in the URL a valid URI?
  // o Is the payload a JSON object?
  //
  PCHECK_URI(entityId, true,  0, NULL, "Entity::id in URL", 400);
  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, kjValueType(orionldState.requestTree->type), 400);


  //
  // Get the entity from mongo (only really need creDate and type of the attributes ...)
  //
  KjNode* dbEntityP = mongocEntityLookup(entityId, NULL, NULL);
  if (dbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  //
  // If entity type is present in the payload body, it must be a String and identical to the entity type in the database.
  // [ It is already extracted (by orionldMhdConnectionTreat) and checked for String ]
  //
  KjNode*     dbTypeNodeP = dbEntityTypeExtract(dbEntityP);
  const char* entityType  = dbTypeNodeP->value.s;

  if (orionldState.payloadTypeNode != NULL)
  {
    if (strcmp(orionldState.payloadTypeNode->value.s, entityType) != 0)
    {
      orionldError(OrionldBadRequestData, "Mismatching Entity::type in payload body", "Does not coincide with the Entity::type in the database", 400);
      return false;
    }
  }


  //
  // In the DB, all attributes are placed under "attrs".
  //
  // First of all, we need to know thast the attribute exists prior to the request - can't PATCH something that doesn't exist
  // But also, to check the validity of the incoming attributes, we need to know the "old" state of the attributes.
  //
  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");
  if (dbAttrsP == NULL)  // This can happen, unfortunately, if the Entity has no attributes - bad DB model ...
    dbAttrsP = kjObject(orionldState.kjsonP, NULL);

  //
  // Loop over incoming attributes
  //
  // The loop will for each incoming attribute:
  // - check validity and remove from orionldState.requestTree if invalid (+ add to error array for response)
  // - Call pCheckAttribute, that expands and normalizes the attribute
  // - Call dbModelFromApiAttribute, that transforms it into a DM Model attribute (DESTRUCTIVE)
  //
  // As we need the incoming (expanded and normalized) list of attributes for Notifications, we need to clone
  // before calling dbModelFromApiAttribute.
  //
  KjNode* updatedV    = kjArray(orionldState.kjsonP, "updated");
  KjNode* notUpdatedV = kjArray(orionldState.kjsonP, "notUpdated");

  KjNode* incomingP   = kjObject(orionldState.kjsonP, NULL);
  KjNode* inAttrP     = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  while (inAttrP != NULL)
  {
    next = inAttrP->next;

    if (strcmp(inAttrP->name, "scope") == 0)
    {
      // Not Implemented: scope - just ignore it
      kjChildRemove(orionldState.requestTree, inAttrP);
      inAttrP = next;
      continue;
    }

    if (strcmp(inAttrP->name, "id") == 0)
    {
      // Can't patch the entity id ...
      attributeNotUpdated(notUpdatedV, "id", "the ID of an entity cannot be altered", NULL);
      kjChildRemove(orionldState.requestTree, inAttrP);
      inAttrP = next;
      continue;
    }

    if (strcmp(inAttrP->name, "type") == 0)
    {
      // Can't patch the entity type ...
      attributeNotUpdated(notUpdatedV, "type", "the TYPE of an entity cannot be altered", NULL);
      kjChildRemove(orionldState.requestTree, inAttrP);
      inAttrP = next;
      continue;
    }

    if ((strcmp(inAttrP->name, "createdAt") == 0) || (strcmp(inAttrP->name, "modifiedAt") == 0))
    {
      // Can't set built-in timestamps
      attributeNotUpdated(notUpdatedV, inAttrP->name, "built-in timestamps are ignored", NULL);
      kjChildRemove(orionldState.requestTree, inAttrP);
      inAttrP = next;
      continue;
    }

    //
    // Does the attribute exist? (Can't be PATCHed otherwise)
    //
    char* shortName = inAttrP->name;

    inAttrP->name = orionldAttributeExpand(orionldState.contextP, inAttrP->name, true, NULL);
    char eqName[512];
    char dotName[512];
    strncpy(dotName, inAttrP->name, sizeof(dotName) - 1);  // dbModelFromApiAttribute destroys the att name (dotForEq)
    strncpy(eqName,  inAttrP->name, sizeof(eqName)  - 1);
    dotForEq(eqName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqName);

    if (dbAttrP == NULL)
    {
      kjChildRemove(orionldState.requestTree, inAttrP);
      attributeNotUpdated(notUpdatedV, shortName, "Attribute not found", NULL);
      inAttrP = next;
      continue;
    }


    //
    // Does the attribute have a type in the DB?
    //
    KjNode* dbAttrTypeP = kjLookup(dbAttrP, "type");
    if (dbAttrTypeP == NULL)
    {
      orionldError(OrionldInternalError, "Database Error", "attribute without type in DB", 500);
      inAttrP = next;
      continue;
    }


    //
    // Is the attribute valid?
    //
    OrionldAttributeType attrTypeFromDb = orionldAttributeType(dbAttrTypeP->value.s);  // Crash if dbAttrTypeP == NULL ... OK somehow ...
    OrionldContextItem*  contextItemP   = NULL;

    if (pCheckAttribute(inAttrP, true, attrTypeFromDb, true, contextItemP) == false)
    {
      kjChildRemove(orionldState.requestTree, inAttrP);
      attributeNotUpdated(notUpdatedV, shortName, orionldState.pd.title, orionldState.pd.detail);
      inAttrP = next;
      continue;
    }

    //
    // inAttrP is now an expanded and normalized API Attribute
    // Before we let dbModelFromApiAttribute destructively transform it into a DB Model Attribute,
    // we clone it and add it to 'incomingP';
    //
    KjNode* apiAttributeP = kjClone(orionldState.kjsonP, inAttrP);
    kjChildAdd(incomingP, apiAttributeP);

    //
    // Transform the incoming attribute into a DB Attribute and save it for a later call to mongoc
    // - Make sure the creDate is taken from dbAttrP and put into inAttrP?
    //
    if (dbModelFromApiAttribute(inAttrP, dbAttrsP, NULL, NULL, NULL, true) == false)
    {
      kjChildRemove(orionldState.requestTree, inAttrP);
      attributeNotUpdated(notUpdatedV, shortName, orionldState.pd.title, orionldState.pd.detail);
      inAttrP = next;
      continue;
    }

    attributeUpdated(updatedV, shortName);
    inAttrP = next;
  }

  kjTreeLog(orionldState.requestTree, "PAE: orionldState.requestTree for mongo");

  // Anything to actually ADD , or was everything BAD ?
  if (orionldState.requestTree->value.firstChildP != NULL)
  {
    if (mongocAttributesAdd(entityId, NULL, orionldState.requestTree, false) == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocAttributesAdd failed", 500);
      return false;
    }

    //
    // For alteration() we need:
    // - Initial DB entity, before the merge  (dbEntityP)
    // - Incoming Entity, expanded and normalized, for watchedAttributes  (incomingP)
    // - Final API Entity, for q, etc AND for the Notifications (finalApiEntityP)
    //     Note; the Final API Entity needs to be Expanded and Normalized - each subscription may have a different context to for compaction
    //
    KjNode* dbEntityCopy    = kjClone(orionldState.kjsonP, dbEntityP);  // dbModelToApiEntity2 is DESTRUCTIVE
    KjNode* finalApiEntityP = dbModelToApiEntity2(dbEntityCopy, false, RF_NORMALIZED, NULL, false, &orionldState.pd);

    if (finalApiEntityP == NULL)
    {
      // There will be no notifications for this update :(  - Well.  cause something important failed ...
      LM_E(("dbModelToApiEntity2: %s: %s", orionldState.pd.title, orionldState.pd.detail));
    }
    else
    {
      attributesMerge(finalApiEntityP, incomingP);

      kjTreeLog(dbEntityP, "PAE: dbEntityP for Alterations");
      kjTreeLog(incomingP, "PAE: incomingP for Alterations");
      kjTreeLog(finalApiEntityP, "PAE: finalApiEntityP for Alterations");

      alteration(entityId, entityType, finalApiEntityP, incomingP, dbEntityP);
    }

    //
    // For TRoE we need:
    // - Incoming Entity, normalized
    //
    kjTreeLog(incomingP, "PAE: incomingP for TRoE");
    orionldState.requestTree = incomingP;
  }
  else
    orionldState.requestTree = NULL;  // Nothing updated, nothing to be done for TRoE

  if (notUpdatedV->value.firstChildP == NULL)
  {
    orionldState.httpStatusCode = 204;
    orionldState.responseTree   = NULL;
  }
  else
  {
    orionldState.httpStatusCode = 207;
    orionldState.responseTree   = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(orionldState.responseTree, updatedV);
    kjChildAdd(orionldState.responseTree, notUpdatedV);
  }

  return true;
}
