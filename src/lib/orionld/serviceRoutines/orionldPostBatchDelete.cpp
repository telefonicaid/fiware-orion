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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityLookupById.h"                   // entityLookupBy_id_Id
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/legacyDriver/legacyPostBatchDelete.h"        // legacyPostBatchDelete
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/mongoc/mongocEntitiesExist.h"                // mongocEntitiesExist
#include "orionld/mongoc/mongocEntitiesDelete.h"               // mongocEntitiesDelete
#include "orionld/serviceRoutines/orionldPostBatchDelete.h"    // Own interface



// ----------------------------------------------------------------------------
//
// eidLookup - lookup an Entity Id in "the rest of the array"
//
static KjNode* eidLookup(KjNode* eidP, const char* entityId)
{
  while (eidP != NULL)
  {
    if ((eidP->type == KjString) && (strcmp(eidP->value.s, entityId) == 0))
      return eidP;
    eidP = eidP->next;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// orionldPostBatchDelete -
//
bool orionldPostBatchDelete(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostBatchDelete();

  //
  // Incoming payload body is an array of Strings - Entity IDs
  // The Strings can't be empty and must be valis URIs
  //
  PCHECK_ARRAY(orionldState.requestTree,       0, NULL, "payload body must be a JSON Array",           400);
  PCHECK_ARRAY_EMPTY(orionldState.requestTree, 0, NULL, "payload body must be a non-empty JSON Array", 400);

  KjNode* outArrayErroredP = kjArray(orionldState.kjsonP, "errors");
  // KjNode* outArrayDeletedP = kjArray(orionldState.kjsonP, "success");

  // Check children for String and valid URI
  KjNode* eidNodeP = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  while (eidNodeP != NULL)
  {
    next = eidNodeP->next;
    if (eidNodeP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Invalid JSON type", "Entity::id must be a JSON String", 400);
      return false;
    }
    else if (pCheckUri(eidNodeP->value.s, "Entity::id", true) == false)
    {
      orionldError(OrionldBadRequestData, "Invalid URI", "Entity::id must be a valid URI", 400);
      return false;
    }

    KjNode* duplicatedP = eidLookup(next, eidNodeP->value.s);
    if (duplicatedP != NULL)  // Duplicated entity id?
    {
      if (duplicatedP == next)
        next = duplicatedP->next;
      kjChildRemove(orionldState.requestTree, duplicatedP);
    }

    eidNodeP = next;
  }

  // Now we have an Array with valid Entity IDs - time to ask mongo if they actually exist
  kjTreeLog(orionldState.requestTree, "Entities to DELETE");
  KjNode* dbEntityIdArray = mongocEntitiesExist(orionldState.requestTree);
  kjTreeLog(dbEntityIdArray, "Existing Entities from DB");

  // Give error for the Entities that don't exist
  KjNode* inEntityIdNodeP = orionldState.requestTree->value.firstChildP;

  while (inEntityIdNodeP != NULL)
  {
    next = inEntityIdNodeP->next;

    char*   entityId = inEntityIdNodeP->value.s;
    KjNode* foundP   = entityLookupBy_id_Id(dbEntityIdArray, entityId, NULL);

    if (foundP == NULL)
    {
      kjChildRemove(orionldState.requestTree, inEntityIdNodeP);
      entityErrorPush(outArrayErroredP, entityId, OrionldResourceNotFound, "Entity Not Found", "Cannot delete entities that do not exist", 404);
    }

    inEntityIdNodeP = next;
  }

  // And those that existed (still present in orionldState.requestTree), delete them!
  mongocEntitiesDelete(orionldState.requestTree);

  if (outArrayErroredP->value.firstChildP == NULL)
    orionldState.httpStatusCode  = 204;
  else
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(response, outArrayErroredP);
    kjChildAdd(response, orionldState.requestTree);
    orionldState.requestTree->name = (char*) "success";

    orionldState.responseTree    = response;
    orionldState.httpStatusCode  = 207;
  }

  orionldState.out.contentType = JSON;
  orionldState.noLinkHeader    = true;

  return true;
}
