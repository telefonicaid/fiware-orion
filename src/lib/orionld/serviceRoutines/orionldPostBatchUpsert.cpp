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
#include <string.h>                                            // strcmp
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityLookupById.h"                   // entityLookupBy_id_Id
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/batchEntityCountAndFirstCheck.h"      // batchEntityCountAndFirstCheck
#include "orionld/common/batchEntityStringArrayPopulate.h"     // batchEntityStringArrayPopulate
#include "orionld/common/batchEntitiesFinalCheck.h"            // batchEntitiesFinalCheck
#include "orionld/common/batchMultipleInstances.h"             // batchMultipleInstances
#include "orionld/common/batchUpdateEntity.h"                  // batchUpdateEntity
#include "orionld/common/batchCreateEntity.h"                  // batchCreateEntity
#include "orionld/common/batchReplaceEntity.h"                 // batchReplaceEntity
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration
#include "orionld/types/StringArray.h"                         // StringArray
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/mongoc/mongocEntitiesQuery.h"                // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntitiesUpsert.h"               // mongocEntitiesUpsert
#include "orionld/legacyDriver/legacyPostBatchUpsert.h"        // legacyPostBatchUpsert
#include "orionld/notifications/alteration.h"                  // alteration
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"    // Own interface



// ----------------------------------------------------------------------------
//
// kjConcatenate - move all children from srcP to the end of destP
//
static KjNode* kjConcatenate(KjNode* destP, KjNode* srcP)
{
  if (destP->value.firstChildP == NULL)
    destP->value.firstChildP = srcP->value.firstChildP;
  else
    destP->lastChild->next   = srcP->value.firstChildP;

  destP->lastChild = srcP->lastChild;

  // Empty srcP
  srcP->value.firstChildP = NULL;
  srcP->lastChild         = NULL;

  return destP;
}



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpsert -
//
// Still to implement
// - datasetId  (don't want to ...)
// - Forwarding (need new registrations for that)
//
bool orionldPostBatchUpsert(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostBatchUpsert();

  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array with objects
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  //
  PCHECK_ARRAY(orionldState.requestTree,       0, NULL, "payload body must be a JSON Array",           400);
  PCHECK_ARRAY_EMPTY(orionldState.requestTree, 0, NULL, "payload body must be a non-empty JSON Array", 400);

  //
  // Prerequisites for URI params:
  // * both 'update' and 'replace' cannot be set in options (replace is default)
  //
  if ((orionldState.uriParamOptions.update == true) && (orionldState.uriParamOptions.replace == true))
  {
    orionldError(OrionldBadRequestData, "URI Param Error", "options: both /update/ and /replace/ present", 400);
    return false;
  }


  //
  // By default, already existing entities are OVERWRITTEN.
  // If ?options=update is used, then already existing entities are to be updated, and in such case we need to
  // extract those to-be-updated entities from the database (the updating algorithm here is according to "Append Attributes).
  // However, for subscriptions, we'll need the old values anyways to help decide whether any notifications are to be sent.
  // So, no other way around it than to extract all the entitiers from the DB :(
  //
  // For the mongoc query, we need a StringArray for the entity IDs, and to set up the StringArray we need to know the number of
  // entities.
  //
  // Very basic error checking is performed in this first loop to count the number of entities in the array.
  // batchEntityCountAndFirstCheck() takes care of that.
  //
  KjNode*      outArrayErroredP = kjArray(orionldState.kjsonP, "errors");
  int          noOfEntities     = batchEntityCountAndFirstCheck(orionldState.requestTree, outArrayErroredP);

  LM_T(LmtSR, ("Number of valid Entities after 1st check-round: %d", noOfEntities));


  //
  // Now that we know the max number of entities (some may drop out after calling pCheckEntity - part of batchEntitiesFinalCheck),
  // we can create the StringArray with the Entity IDs
  // We need the StringArray 'eIdArray' to query mongo, to make sure the Entities already exist, and for the possible merge that comes after.
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
  // Important to remember about pagination here!!!
  // Default value for the "limit" is 20, so, if I don't do anything about that,
  // I'll only get the first 20 entities, and ... I need them ALL.
  //
  orionldState.uriParams.limit  = noOfEntities;
  orionldState.uriParams.offset = 0;
  KjNode* dbEntityArray = mongocEntitiesQuery(NULL, &eIdArray, NULL, NULL, NULL, NULL, NULL, NULL, false);
  if (dbEntityArray == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "error querying the database for entities", 500);
    return false;
  }

  //
  // Finally we have everything we need to 100% CHECK the incoming entities
  //
  noOfEntities = batchEntitiesFinalCheck(orionldState.requestTree, outArrayErroredP, dbEntityArray, orionldState.uriParamOptions.update, false, false);
  LM_T(LmtSR, ("Number of valid Entities after 3rd check-round: %d", noOfEntities));

  KjNode* outArrayCreatedP  = kjArray(orionldState.kjsonP, "created");  // For the HTTP response payload body
  KjNode* outArrayUpdatedP  = kjArray(orionldState.kjsonP, "updated");  // For the HTTP response payload body

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
  // 7. dbCreateArray        - array of finalDbEntityP that are to be created - for mongoc
  // 8. dbUpdateArray        - array of finalDbEntityP that are to be updated - for mongoc
  //
  // TRoE needs         incomingEntityP
  // Alteration needs   incomingEntityP (to check for matching subscriptions)
  // Alteration needs   finalApiEntityP (for the notification - filter attrs etc)
  // mongoc needs       finalDbEntityP
  //
  KjNode* dbCreateArray = kjArray(orionldState.kjsonP, NULL);  // For mongo
  KjNode* dbUpdateArray = kjArray(orionldState.kjsonP, NULL);  // For mongo

  KjNode* next;
  KjNode* inEntityP = orionldState.requestTree->value.firstChildP;

  while (inEntityP != NULL)
  {
    next = inEntityP->next;

    KjNode*  idNodeP            = kjLookup(inEntityP, "id");    // pCheckEntity assures "id" is present (as a String and not named "@id")
    KjNode*  typeNodeP          = kjLookup(inEntityP, "type");  // pCheckEntity assures "type" if present is a String and not named "@type"
    char*    entityId           = idNodeP->value.s;
    char*    entityType         = (typeNodeP != NULL)? typeNodeP->value.s : NULL;
    KjNode*  originalDbEntityP  = entityLookupBy_id_Id(dbEntityArray, entityId, NULL);
    KjNode*  finalDbEntityP;
    KjNode*  dbArray            = dbUpdateArray;            // Points to either dbUpdateArray or dbCreateArray
    bool     multipleEntities   = false;

    if (batchMultipleInstances(entityId, outArrayUpdatedP, outArrayCreatedP) == true)
    {
      multipleEntities = true;

      if (originalDbEntityP == NULL)
        dbArray = dbCreateArray;

      //
      // If REPLACE, we need the Final API Entity before the REPLACE Operation to see if any attributes have been deleted
      // (Present in the old Final API Entity but not present in the REPLACing Entity)
      //
      // The current state needs to be overwritten
      // We can do that by:
      // 1. Remove the item in its DB Array (either dbCreateArray or dbUpdateArray)
      // 2. Let the function continue so a new item is inserted in the DB Array
      //
      KjNode* dbArrayItemP = entityLookupBy_id_Id(dbArray, entityId, NULL);
      if (dbArrayItemP == NULL)
        LM_E(("MI: Internal Error (multiple instance entity '%s' not found in DB Array)", entityId));
      else
      {
        kjChildRemove(dbArray, dbArrayItemP);

        if (orionldState.uriParamOptions.update == true)
          originalDbEntityP = dbArrayItemP;   // The previous "db entity" is now the base for this update
      }
    }

    if (originalDbEntityP == NULL)  // The entity did not exist before - CREATION
    {
      finalDbEntityP = batchCreateEntity(inEntityP, entityId, entityType, multipleEntities);

      if (finalDbEntityP != NULL)
      {
        kjChildAdd(dbCreateArray, finalDbEntityP);
        if (multipleEntities == false)  // Cause, "if multipleEntities == true", then the entityID is in the outArrayCreatedP array already
          entitySuccessPush(outArrayCreatedP, entityId);
      }
    }
    else
    {
      if (orionldState.uriParamOptions.update == false)
      {
        KjNode* entityCreDateNodeP = kjLookup(originalDbEntityP, "creDate");  // I'd really prefer dbModelFromApiEntity to fix "creDate"
        double  entityCreDate      = (entityCreDateNodeP != NULL)? entityCreDateNodeP->value.f : orionldState.requestTime;

        finalDbEntityP = batchReplaceEntity(inEntityP, entityId, entityType, entityCreDate);
      }
      else
        finalDbEntityP = batchUpdateEntity(inEntityP, originalDbEntityP, false);

      if (finalDbEntityP != NULL)
      {
        kjChildAdd(dbArray, finalDbEntityP);
        if (multipleEntities == false)
          entitySuccessPush(outArrayUpdatedP, entityId);
      }
    }

    if (finalDbEntityP == NULL)
    {
      inEntityP = next;
      continue;
    }

    //
    // Alterations need the complete API entity (I might change that for the complete DB Entity ...)
    // dbModelToApiEntity is DESTRUCTIVE, so I need to clone the 'finalDbEntityP' first
    //
    KjNode* dbEntityCopy      = kjClone(orionldState.kjsonP, finalDbEntityP);
    KjNode* finalApiEntityP   = dbModelToApiEntity(dbEntityCopy, false, entityId);
    KjNode* initialDbEntityP  = NULL;  // FIXME: initialDbEntity might not be NULL

    alteration(entityId, entityType, finalApiEntityP, inEntityP, initialDbEntityP);

    inEntityP = next;
  }

  //
  // Any correct Entity to be created/updated??
  //
  if ((dbCreateArray->value.firstChildP != NULL) || (dbUpdateArray->value.firstChildP != NULL))
  {
    int r = mongocEntitiesUpsert(dbCreateArray, dbUpdateArray);

    if (r == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntitiesUpsert failed", 500);
      return false;
    }
  }

  //
  // Returning the three arrays, or not ...
  //
  // 1. The broker returns 201 if there are no errors
  //    - updatedArrayP EMPTY
  //    - createdArrayP NOT EMPTY
  //
  // 2. The broker returns 204 if all entities have been updated:
  //    - errorsArrayP  EMPTY
  //    - createdArrayP EMPTY
  //    - updatedArrayP NOT EMPTY
  //
  // 3. Else, 207 is returned
  //
  bool    noErrors    = (outArrayErroredP->value.firstChildP == NULL);
  bool    noCreations = (outArrayCreatedP->value.firstChildP == NULL);

  if (noErrors)
  {
    if (noCreations)  // Only Updates - 204
    {
      orionldState.httpStatusCode = 204;
      orionldState.responseTree   = NULL;
    }
    else  // Some Creations (and perhaps Updates) - 201 with creation array as body (and ignoring the updates)
    {
      orionldState.httpStatusCode = 201;
      orionldState.responseTree   = outArrayCreatedP;
    }
  }
  else  // There are errors - 207
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    orionldState.httpStatusCode  = 207;

    kjConcatenate(outArrayCreatedP, outArrayUpdatedP);
    outArrayCreatedP->name = (char*) "success";
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
