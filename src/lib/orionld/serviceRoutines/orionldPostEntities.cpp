/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include <unistd.h>                                              // NULL

#include "logMsg/logMsg.h"                                       // LM_*

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjClone.h"                                       // kjClone
}

#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/performance.h"                          // PERFORMANCE
#include "orionld/legacyDriver/legacyPostEntities.h"             // legacyPostEntities
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pCheckEntityId.h"                 // pCheckEntityId
#include "orionld/payloadCheck/pCheckEntityType.h"               // pCheckEntityType
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/dbModel/dbModelFromApiEntity.h"                // dbModelFromApiEntity
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityInsert.h"                   // mongocEntityInsert
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyPostEntities();

  char*    entityId;
  char*    entityType;

  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "To create an Entity, a JSON OBJECT describing the entity must be provided", 400);

  if (pCheckEntityId(orionldState.payloadIdNode,     true, &entityId)   == false)   return false;
  if (pCheckEntityType(orionldState.payloadTypeNode, true, &entityType) == false)   return false;


  //
  // If the entity already exists, a "409 Conflict" is returned
  //
  if (mongocEntityLookup(entityId) != NULL)
  {
    orionldError(OrionldAlreadyExists, "Entity already exists", entityId, 409);
    return false;
  }

  //
  // NOTE
  //   payloadParseAndExtractSpecialFields() from orionldMhdConnectionTreat() decouples the entity id and type
  //   from the payload body, so, the entity type is not expanded by pCheckEntity()
  //   The expansion is instead done by payloadTypeNodeFix, called by orionldMhdConnectionTreat
  //

  //
  // Check and fix the incoming payload (entity)
  //
  if (pCheckEntity(orionldState.requestTree, false, NULL) == false)
    return false;

  // dbModelFromApiEntity destroys the tree, need to make a copy for notifications
  KjNode* apiEntityP = kjClone(orionldState.kjsonP, orionldState.requestTree);

  // Entity id and type was removed - they need to go back
  orionldState.payloadIdNode->next   = orionldState.payloadTypeNode;
  orionldState.payloadTypeNode->next = apiEntityP->value.firstChildP;
  apiEntityP->value.firstChildP      = orionldState.payloadIdNode;


  //
  // The current shape of the incoming tree is now fit for TRoE, while it still needs to be adjusted for mongo,
  // If TRoE is enable we clone it here, for later use in TRoE processing
  //
  KjNode* cloneForTroeP = NULL;
  if (troe)
    cloneForTroeP = kjClone(orionldState.kjsonP, apiEntityP);  // apiEntityP contains entity ID and TYPE

  if (dbModelFromApiEntity(orionldState.requestTree, NULL, true, orionldState.payloadIdNode->value.s, orionldState.payloadTypeNode->value.s) == false)
  {
    //
    // Not calling orionldError as a better error message is overwritten if I do.
    // Once we have "Error Stacking", orionldError should be called.
    //
    // orionldError(OrionldInternalError, "Internal Error", "Unable to convert API Entity into DB Model Entity", 500);

    return false;
  }

  KjNode* dbEntityP = orionldState.requestTree;  // More adecuate to talk about DB-Entity from here on

  // datasets?
  if (orionldState.datasets != NULL)
    kjChildAdd(dbEntityP, orionldState.datasets);

  // Ready to send it to the database
  if (mongocEntityInsert(dbEntityP, entityId) == false)
  {
    orionldError(OrionldInternalError, "Database Error", "mongocEntityInsert failed", 500);
    return false;
  }

  //
  // Prepare for notifications
  //
  orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  orionldState.alterations->entityId          = entityId;
  orionldState.alterations->entityType        = entityType;
  orionldState.alterations->patchTree         = NULL;
  orionldState.alterations->dbEntityP         = NULL;
  orionldState.alterations->patchedEntity     = apiEntityP;  // entity id, createdAt, modifiedAt ...
  orionldState.alterations->alteredAttributes = 0;
  orionldState.alterations->alteredAttributeV = NULL;
  orionldState.alterations->next              = NULL;

  // All good
  orionldState.httpStatusCode = 201;
  httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);

  if (cloneForTroeP != NULL)
    orionldState.requestTree = cloneForTroeP;

  return true;
}
