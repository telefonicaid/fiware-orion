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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <string>
#include <vector>

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityDelete.h"                   // mongocEntityDelete
#include "orionld/notifications/orionldAlterations.h"            // orionldAlterations
#include "orionld/serviceRoutines/orionldDeleteEntity.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteEntity -
//
bool orionldDeleteEntity(void)
{
  char* entityId            = orionldState.wildcard[0];
  char* entityTypeExpanded  = orionldState.uriParams.type;
  char* entityTypeCompacted = NULL;

  // Make sure the Entity ID is a valid URI
  PCHECK_URI(entityId, true, 0, "Invalid Entity ID", "Must be a valid URI", 400);

  if (orionldState.uriParams.type != NULL)
    entityTypeExpanded = orionldContextItemExpand(orionldState.contextP, orionldState.uriParams.type, true, NULL);

  //
  // GET the entity locally
  //   FIXME: Overkill to extract the entire entity from the DB
  //
  KjNode* dbEntityP  = mongocEntityLookup(entityId, entityTypeExpanded, NULL, NULL);

  //
  // More info on Entity Type
  //
  // - If given as URI param, we must expand it according to the @context
  // - If entity type not given as URI param, but the entity is found in the DB - extract it (the entity type) AND compact it
  // - Also, if given as URI param, we might need to compact it back to shortname according to the @context
  //
  if ((orionldState.uriParams.type == NULL) && (dbEntityP != NULL))
  {
    KjNode* _idP   = kjLookup(dbEntityP, "_id");
    KjNode*  typeP = (_idP != NULL)? kjLookup(_idP, "type") : NULL;

    entityTypeExpanded  = (typeP != NULL)? typeP->value.s : NULL;  // Always Expanded in DB
  }

  if (entityTypeExpanded != NULL)
    entityTypeCompacted = orionldContextItemAliasLookup(orionldState.contextP, entityTypeExpanded, NULL, NULL);

  //
  // Create the alteration for notifications
  //
  orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  bzero(orionldState.alterations, sizeof(OrionldAlteration));
  orionldState.alterations->entityId   = entityId;
  orionldState.alterations->entityType = entityTypeExpanded;

  //
  // Create a payload body to represent the deletion (to be sent as notification later
  //
  KjNode* apiEntityP      = kjObject(orionldState.kjsonP, NULL);  // Used for notifications, if needed
  KjNode* idNodeP         = kjString(orionldState.kjsonP, "id", entityId);
  KjNode* deletedAtNodeP  = kjString(orionldState.kjsonP, "deletedAt", orionldState.requestTimeString);

  kjChildAdd(apiEntityP, idNodeP);
  if (entityTypeCompacted != NULL)
  {
    KjNode* typeNodeP    = kjString(orionldState.kjsonP, "type", entityTypeCompacted);
    kjChildAdd(apiEntityP, typeNodeP);
  }
  kjChildAdd(apiEntityP, deletedAtNodeP);

  orionldState.alterations->finalApiEntityP = apiEntityP;


  //
  // Error if the entity is not found locally (needs to be changed for Distributed Operations)
  //
  if (dbEntityP == NULL)
  {
    // FIXME: for Distributed operations, I need to know the reg-matches here. Only 404 if no reg-matches
    orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
    return false;
  }

  //
  // Delete the entity in the local DB
  //
  if (mongocEntityDelete(entityId) == false)
  {
    orionldError(OrionldInternalError, "Database Error", "mongocEntityDelete failed", 500);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;  // 204

  if (troe)
  {
    LM(("Mark the entity as DELETED in TRoE DB"));  // OR, is this already taken care of?
  }

  return true;
}
