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
#include <unistd.h>                                              // NULL, gethostname

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

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
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityInsert.h"                   // mongocEntityInsert
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/distOpListRelease.h"                // distOpListRelease
#include "orionld/forwarding/distOpSuccess.h"                    // distOpSuccess
#include "orionld/forwarding/distOpFailure.h"                    // distOpFailure
#include "orionld/forwarding/distOpRequests.h"                   // distOpRequests
#include "orionld/forwarding/distOpResponses.h"                  // distOpResponses
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyPostEntities();

  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "To create an Entity, a JSON OBJECT defining the entity must be provided", 400);

  char*  entityId;
  char*  entityType;

  if (pCheckEntityId(orionldState.payloadIdNode,     true, &entityId)   == false)   return false;
  if (pCheckEntityType(orionldState.payloadTypeNode, true, &entityType) == false)   return false;


  //
  // Check and fix the incoming payload (entity)
  //
  if (pCheckEntity(orionldState.requestTree, false, NULL) == false)
    return false;

  KjNode*  responseBody  = kjObject(orionldState.kjsonP, NULL);                  // Only used if 207 response
  KjNode*  entityIdNodeP = kjString(orionldState.kjsonP, "entityId", entityId);  // Only used if 207 response

  kjChildAdd(responseBody, entityIdNodeP);

  DistOp*  distOpList = NULL;
  if (orionldState.distributed)
    distOpList = distOpRequests(entityId, entityType, DoCreateEntity);

  //
  // If the entity already exists, a "409 Conflict" is returned, either complete or as part of a 207
  //
  if (orionldState.requestTree != NULL)
  {
    if (mongocEntityLookup(entityId, NULL, NULL, NULL) != NULL)
    {
      if (distOpList == NULL)  // Purely local request
      {
        orionldError(OrionldAlreadyExists, "Entity already exists", entityId, 409);
        return false;
      }
      else
      {
        distOpFailure(responseBody, NULL, OrionldAlreadyExists, "Entity already exists", entityId, 404);
        goto awaitDoResponses;
      }
    }

    //
    // NOTE
    //   payloadParseAndExtractSpecialFields() from orionldMhdConnectionTreat() decouples the entity id and type
    //   from the payload body, so, the entity type is not expanded by pCheckEntity()
    //   The expansion is instead done by payloadTypeNodeFix, called by orionldMhdConnectionTreat
    //

    // dbModelFromApiEntity destroys the tree, need to make a copy for notifications
    KjNode* apiEntityP = kjClone(orionldState.kjsonP, orionldState.requestTree);

    // Entity id and type was removed - they need to go back
    orionldState.payloadIdNode->next   = orionldState.payloadTypeNode;
    orionldState.payloadTypeNode->next = apiEntityP->value.firstChildP;
    apiEntityP->value.firstChildP      = orionldState.payloadIdNode;


    //
    // The current shape of the incoming tree is now fit for TRoE, while it still needs to be adjusted for mongo,
    // If TRoE is enabled we clone it here, for later use in TRoE processing
    //
    // The same tree (read-only) might also be necessary for a 207 response in a distributed operation.
    // So, if anythis has been forwarded, the clone is made regardless whether TRoE is enabled.
    //
    KjNode* cloneForTroeP = NULL;
    if ((troe == true) || (distOpList != NULL))
      cloneForTroeP = kjClone(orionldState.kjsonP, apiEntityP);  // apiEntityP contains entity ID and TYPE

    if (dbModelFromApiEntity(orionldState.requestTree, NULL, true, orionldState.payloadIdNode->value.s, orionldState.payloadTypeNode->value.s) == false)
    {
      //
      // Not calling orionldError as a better error message is overwritten if I do.
      // Once we have "Error Stacking", orionldError should be called.
      //
      // orionldError(OrionldInternalError, "Internal Error", "Unable to convert API Entity into DB Model Entity", 500);

      if (distOpList == NULL)  // Purely local request
        return false;
      else
      {
        distOpFailure(responseBody, NULL, OrionldInternalError, "Internal Error", "dbModelFromApiEntity failed", 500);
        goto awaitDoResponses;
      }
    }

    KjNode* dbEntityP = orionldState.requestTree;  // More adequate to talk about DB-Entity from here on

    // datasets?
    if (orionldState.datasets != NULL)
      kjChildAdd(dbEntityP, orionldState.datasets);

    // Ready to send it to the database
    if (mongocEntityInsert(dbEntityP, entityId) == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntityInsert failed", 500);
      if (distOpList == NULL)  // Purely local request
        return false;
      else
      {
        distOpFailure(responseBody, NULL, OrionldInternalError, "Database Error", "mongocEntityInsert failed", 500);
        goto awaitDoResponses;
      }
    }
    else if (distOpList != NULL)  // NOT Purely local request
    {
      //
      // Need to call distOpSuccess here. Only, I don't have a DistOp object for local ...
      // Only the "DistOp::body" is used in distOpSuccess so I can fix it:
      //
      DistOp local;
      local.body = cloneForTroeP;
      distOpSuccess(responseBody, &local);
    }

    //
    // Prepare for notifications
    //
    orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
    orionldState.alterations->entityId          = entityId;
    orionldState.alterations->entityType        = entityType;
    orionldState.alterations->inEntityP         = apiEntityP;
    orionldState.alterations->dbEntityP         = NULL;
    orionldState.alterations->finalApiEntityP   = apiEntityP;  // entity id, createdAt, modifiedAt ...
    orionldState.alterations->alteredAttributes = 0;
    orionldState.alterations->alteredAttributeV = NULL;
    orionldState.alterations->next              = NULL;

    // All good
    orionldState.httpStatusCode = 201;
    httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);

    if (cloneForTroeP != NULL)
      orionldState.requestTree = cloneForTroeP;

    if (distOpList == NULL)  // Purely local request
      return true;
  }

 awaitDoResponses:
  if (distOpList != NULL)
    distOpResponses(distOpList, responseBody);

  if (kjLookup(responseBody, "failure") != NULL)
  {
    orionldState.httpStatusCode = 207;
    orionldState.responseTree   = responseBody;
  }
  else
  {
    // All good
    if (orionldState.httpStatusCode != 201)  // Cause, this might be done already - inside local processing of entity
    {
      orionldState.httpStatusCode = 201;
      httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);
    }
  }

  if (orionldState.curlDoMultiP != NULL)
    distOpListRelease(distOpList);

  return true;
}
