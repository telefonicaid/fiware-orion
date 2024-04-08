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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/entityLookupById.h"                   // entityLookupBy_id_Id
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/batchEntityCountAndFirstCheck.h"      // batchEntityCountAndFirstCheck
#include "orionld/common/batchEntityStringArrayPopulate.h"     // batchEntityStringArrayPopulate
#include "orionld/common/batchEntitiesFinalCheck.h"            // batchEntitiesFinalCheck
#include "orionld/common/batchMultipleInstances.h"             // batchMultipleInstances
#include "orionld/common/batchUpdateEntity.h"                  // batchUpdateEntity
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/legacyDriver/legacyPostBatchUpdate.h"        // legacyPostBatchUpdate
#include "orionld/mongoc/mongocEntitiesQuery.h"                // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntitiesUpsert.h"               // mongocEntitiesUpsert
#include "orionld/notifications/alteration.h"                  // alteration
#include "orionld/serviceRoutines/orionldPostBatchUpdate.h"    // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpdate -
//
// Still to implement
// - datasetId  (don't want to ...)
// - Forwarding (need new registrations for that)
// - And EVERYTHING ELSE !!! :)
//
bool orionldPostBatchUpdate(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostBatchUpdate();

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
  // We need the StringArray 'eIdArray' to query mongo, to make sure the Entities already exist, and for the merge that comes after.
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
  KjNode* dbEntityArray = mongocEntitiesQuery(NULL, &eIdArray, NULL, NULL, NULL, NULL, NULL, NULL, false, false);
  if (dbEntityArray == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "error querying the database for entities", 500);
    return false;
  }

  //
  // Finally we have everything we need to 100% CHECK the incoming entities
  //
  noOfEntities = batchEntitiesFinalCheck(orionldState.requestTree, outArrayErroredP, dbEntityArray, orionldState.uriParamOptions.update, true, false);
  LM_T(LmtSR, ("Number of valid Entities after 3rd check-round: %d", noOfEntities));


  //
  // Looping over all the accepted entities
  //
  // 1. orionldState.requestTree must remain untouched (as it came out of pCheckEntity)
  //
  // 2. inEntityP            - current entity in orionldState.requestTree
  // 3. originalDbEntityP    - as the entity was in the DB before this request
  // 4. originalApiEntityP   - as the entity was in API format before this request  (not sure it's necessary)
  // 5. finalApiEntityP      - merged entity, in API format
  // 6. finalDbEntityP       - as is goes to mongoc
  // 7. dbUpdateArray        - array of finalDbEntityP that are to be updated - for mongoc (none are created)
  //
  // TRoE needs         incomingEntityP
  // Alteration needs   incomingEntityP (to check for matching subscriptions)
  // Alteration needs   finalApiEntityP (for the notification - filter attrs etc)
  // mongoc needs       finalDbEntityP
  //
  KjNode* outArrayUpdatedP  = kjArray(orionldState.kjsonP, "success");          // For the HTTP response payload body
  KjNode* dbUpdateArray     = kjArray(orionldState.kjsonP, NULL);               // For mongo
  KjNode* inEntityP         = orionldState.requestTree->value.firstChildP;
  bool    ignore            = orionldState.uriParamOptions.noOverwrite == true;
  KjNode* next;

  while (inEntityP != NULL)
  {
    next = inEntityP->next;

    KjNode*  idNodeP            = kjLookup(inEntityP, "id");    // pCheckEntity assures "id" is present (as a String and not named "@id")
    KjNode*  typeNodeP          = kjLookup(inEntityP, "type");  // pCheckEntity assures "type" if present is a String and not named "@type"
    char*    entityId           = idNodeP->value.s;
    char*    entityType         = (typeNodeP != NULL)? typeNodeP->value.s : NULL;
    KjNode*  originalDbEntityP  = entityLookupBy_id_Id(dbEntityArray, entityId, NULL);
    bool     multipleEntities   = false;
    KjNode*  finalDbEntityP;

    if (batchMultipleInstances(entityId, outArrayUpdatedP, NULL) == true)
    {
      multipleEntities = true;

      KjNode* dbArrayItemP = entityLookupBy_id_Id(dbUpdateArray, entityId, NULL);
      if (dbArrayItemP == NULL)
        LM_E(("MI: Internal Error (multiple instance entity '%s' not found in DB Array)", entityId));
      else
      {
        kjChildRemove(dbUpdateArray, dbArrayItemP);  // Soon to be replaced by a newer one
        originalDbEntityP = dbArrayItemP;            // The previous "db entity" is now the base for this update
      }
    }

    finalDbEntityP = batchUpdateEntity(inEntityP, originalDbEntityP, ignore);

    if (finalDbEntityP != NULL)
    {
      if (multipleEntities == false)
        entitySuccessPush(outArrayUpdatedP, entityId);

      kjChildAdd(dbUpdateArray, finalDbEntityP);

      //
      // Alterations need the complete API entity (I might change that for the complete DB Entity ...)
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
  // Any correct Entity to be updated??
  //
  if (dbUpdateArray->value.firstChildP != NULL)
  {
    int r = mongocEntitiesUpsert(NULL, dbUpdateArray);

    if (r == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntitiesUpsert failed", 500);
      return false;
    }
  }

  //
  // Returning the two arrays, or nothing at all (204) ...
  //
  // 1. The broker returns 204 if all entities have been updated:
  //    - errorsArrayP  EMPTY
  //    - updatedArrayP NOT EMPTY
  //
  // 2. Else, 207 is returned
  //
  bool noErrors = (outArrayErroredP->value.firstChildP == NULL);

  if (noErrors)
  {
    orionldState.httpStatusCode = 204;
    orionldState.responseTree   = NULL;
  }
  else  // There are errors - 207
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    orionldState.httpStatusCode  = 207;

    kjChildAdd(response, outArrayUpdatedP);
    kjChildAdd(response, outArrayErroredP);
    orionldState.responseTree = response;
  }

  orionldState.out.contentType = MT_JSON;
  orionldState.noLinkHeader    = true;

  if ((orionldState.tenantP != &tenant0) && (orionldState.httpStatusCode != 204))
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);

  return true;
}
