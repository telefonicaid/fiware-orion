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
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/batchEntityCountAndFirstCheck.h"      // batchEntityCountAndFirstCheck
#include "orionld/common/batchEntityStringArrayPopulate.h"     // batchEntityStringArrayPopulate
#include "orionld/common/batchEntitiesFinalCheck.h"            // batchEntitiesFinalCheck
#include "orionld/common/batchCreateEntity.h"                  // batchCreateEntity
#include "orionld/common/batchMultipleInstances.h"             // batchMultipleInstances
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/legacyDriver/legacyPostBatchCreate.h"        // legacyPostBatchCreate
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/mongoc/mongocEntitiesQuery.h"                // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntitiesUpsert.h"               // mongocEntitiesUpsert
#include "orionld/notifications/alteration.h"                  // alteration
#include "orionld/serviceRoutines/orionldPostBatchCreate.h"    // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostBatchCreate -
//
bool orionldPostBatchCreate(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostBatchCreate();

  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array with objects
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  //
  PCHECK_ARRAY(orionldState.requestTree,       0, NULL, "payload body must be a JSON Array",           400);
  PCHECK_ARRAY_EMPTY(orionldState.requestTree, 0, NULL, "payload body must be a non-empty JSON Array", 400);

  KjNode*      outArrayErroredP = kjArray(orionldState.kjsonP, "errors");
  int          noOfEntities     = batchEntityCountAndFirstCheck(orionldState.requestTree, outArrayErroredP);

  LM_T(LmtSR, ("Number of valid Entities after 1st check-round: %d", noOfEntities));

  //
  // Now that we know the max number of entities (some may drop out after calling pCheckEntity - part of entitiesFinalCheck),
  // we can create the StringArray with the Entity IDs.
  // We need the StringArray 'eIdArray' to query mongo, to make sure the Entities don't exist already.
  //
  StringArray  eIdArray;

  eIdArray.items = noOfEntities;
  eIdArray.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * noOfEntities);

  if (eIdArray.array == NULL)
  {
    orionldError(OrionldInternalError, "Out of memory", "allocating StringArray for entity ids", 500);
    return false;
  }

  //
  // We have the StringArray (eIdArray), so, now we can loop through the incoming array of entities and populate eIdArray
  // (extract the entity ids) later to be used by mongocEntitiesQuery().
  //
  noOfEntities = batchEntityStringArrayPopulate(orionldState.requestTree, &eIdArray, outArrayErroredP, false);
  LM_T(LmtSR, ("Number of valid Entities after 2nd check-round: %d", noOfEntities));

  //
  // The entity id array is ready - time to query mongo
  //
  // FIXME: Use simpler mongoc function - we only need to know whether the entities exist or not
  //
  KjNode* dbEntityArray = mongocEntitiesQuery(NULL, &eIdArray, NULL, NULL, NULL, NULL, NULL, NULL, false, false);
  if (dbEntityArray == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "error querying the database for entities", 500);
    return false;
  }

  //
  // Finally we have everything we need to 100% CHECK the incoming entities
  //
  noOfEntities = batchEntitiesFinalCheck(orionldState.requestTree, outArrayErroredP, dbEntityArray, orionldState.uriParamOptions.update, false, true);
  LM_T(LmtSR, ("Number of valid Entities after 3rd check-round: %d", noOfEntities));


  //
  // Looping over all the accepted entities
  //
  // 1. orionldState.requestTree must remain untouched (as it came out of pCheckEntity)
  //
  // 2. inEntityP            - current entity in orionldState.requestTree
  // 3. finalDbEntityP       - as is goes to mongoc
  // 4. dbCreateArray        - array of finalDbEntityP that are to be created - for mongoc
  //
  // TRoE needs         incomingEntityP
  // Alteration needs   incomingEntityP (to check for matching subscriptions)
  // Alteration needs   finalApiEntityP (for the notification - filter attrs etc)
  // mongoc needs       finalDbEntityP
  //
  KjNode* outArrayCreatedP  = kjArray(orionldState.kjsonP, "success");          // For the HTTP response payload body
  KjNode* dbCreateArray     = kjArray(orionldState.kjsonP, NULL);               // For mongo
  KjNode* inEntityP         = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  while (inEntityP != NULL)
  {
    next = inEntityP->next;

    KjNode*  idNodeP            = kjLookup(inEntityP, "id");    // pCheckEntity assures "id" is present (as a String and not named "@id")
    KjNode*  typeNodeP          = kjLookup(inEntityP, "type");  // pCheckEntity assures "type" if present is a String and not named "@type"
    char*    entityId           = idNodeP->value.s;
    char*    entityType         = (typeNodeP != NULL)? typeNodeP->value.s : NULL;
    KjNode*  finalDbEntityP;

    if (batchMultipleInstances(entityId, outArrayCreatedP, NULL) == true)
    {
      LM_W(("Got another instance of entity '%s' - that's an error", entityId));
      entityErrorPush(outArrayErroredP, entityId, OrionldAlreadyExists, "Entity already exists", "Created as part of the same request", 409);
      kjChildRemove(orionldState.requestTree, inEntityP);
      inEntityP = next;
      continue;
    }

    finalDbEntityP = batchCreateEntity(inEntityP, entityId, entityType, false);

    if (finalDbEntityP != NULL)
    {
      kjChildAdd(dbCreateArray, finalDbEntityP);
      entitySuccessPush(outArrayCreatedP, entityId);

      //
      // Alterations:
      // dbModelToApiEntity is DESTRUCTIVE, so I need to clone the 'finalDbEntityP' first
      //
      KjNode* dbEntityCopy      = kjClone(orionldState.kjsonP, finalDbEntityP);
      KjNode* finalApiEntityP   = dbModelToApiEntity(dbEntityCopy, false, entityId);
      KjNode* initialDbEntityP  = NULL;  // FIXME: initialDbEntity might not be NULL

      alteration(entityId, entityType, finalApiEntityP, inEntityP, initialDbEntityP);
    }

    inEntityP = next;
  }

  //
  // Any correct Entity to be created??
  //
  if (dbCreateArray->value.firstChildP != NULL)
  {
    int r = mongocEntitiesUpsert(dbCreateArray, NULL);

    if (r == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntitiesUpsert failed", 500);
      return false;
    }
  }


  //
  // Returning 201 or 207
  //
  bool noErrors = (outArrayErroredP->value.firstChildP == NULL);

  if (noErrors)
  {
    orionldState.httpStatusCode = 201;
    orionldState.responseTree   = outArrayCreatedP;
  }
  else
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    orionldState.httpStatusCode  = 207;
    kjChildAdd(response, outArrayCreatedP);
    kjChildAdd(response, outArrayErroredP);
    orionldState.responseTree = response;
  }

  orionldState.out.contentType = MT_JSON;
  orionldState.noLinkHeader    = true;

  if ((orionldState.tenantP != &tenant0) && (orionldState.httpStatusCode != 204))
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);

  return true;
}
