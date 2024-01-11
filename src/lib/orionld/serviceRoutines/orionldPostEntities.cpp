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
#include <strings.h>                                             // bzero

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

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/performance.h"                          // PERFORMANCE
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/http/httpHeaderLocationAdd.h"                  // httpHeaderLocationAdd
#include "orionld/legacyDriver/legacyPostEntities.h"             // legacyPostEntities
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pCheckEntityId.h"                 // pCheckEntityId
#include "orionld/payloadCheck/pCheckEntityType.h"               // pCheckEntityType
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/dbModel/dbModelFromApiEntity.h"                // dbModelFromApiEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityInsert.h"                   // mongocEntityInsert
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/distOp/distOpRequests.h"                       // distOpRequests
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/kjTree/kjSort.h"                               // kjStringArraySort
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// -----------------------------------------------------------------------------
//
// sysAttrsToEntity -
//
void sysAttrsToEntity(KjNode* apiEntityP)
{
  KjNode* createdAtP  = kjString(orionldState.kjsonP, "createdAt",  orionldState.requestTimeString);
  KjNode* modifiedAtP = kjString(orionldState.kjsonP, "modifiedAt", orionldState.requestTimeString);

  kjChildAdd(apiEntityP, createdAtP);
  kjChildAdd(apiEntityP, modifiedAtP);

  // Loop over all attributes
  for (KjNode* attrP = apiEntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    // Not an object?  Not an attribute!  (Arrays(datasetId) comes later)
    if (attrP->type != KjObject)
      continue;

    KjNode* createdAtP  = kjString(orionldState.kjsonP, "createdAt",  orionldState.requestTimeString);
    KjNode* modifiedAtP = kjString(orionldState.kjsonP, "modifiedAt", orionldState.requestTimeString);

    kjChildAdd(attrP, createdAtP);
    kjChildAdd(attrP, modifiedAtP);
  }
}



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
    distOpList = distOpRequests(entityId, entityType, DoCreateEntity, orionldState.requestTree);

  bool    emptyEntity    = ((orionldState.requestTree == NULL)  || (orionldState.requestTree->value.firstChildP == NULL));
  KjNode* cloneForTroeP  = NULL;
  KjNode* apiEntityP     = NULL;
  KjNode* dbEntityP      = NULL;

  //
  // If the entity already exists, a "409 Conflict" is returned, either complete or as part of a 207
  //
  dbEntityP = mongocEntityLookup(entityId, NULL, NULL, NULL, NULL);
  if (dbEntityP != NULL)
  {
    if (distOpList == NULL)  // Purely local request
    {
      orionldError(OrionldAlreadyExists, "Entity already exists", entityId, 409);
      return false;
    }
    else
    {
      distOpFailure(responseBody, NULL, "Entity already exists", entityId, 404, NULL);
      goto awaitDoResponses;
    }
  }

  //
  // NOTE
  //   payloadParseAndExtractSpecialFields() from mhdConnectionTreat() decouples the entity id and type
  //   from the payload body, so, the entity type is not expanded by pCheckEntity()
  //   The expansion is instead done by payloadTypeNodeFix, called by mhdConnectionTreat
  //

  // dbModelFromApiEntity destroys the tree, need to make a copy for notifications and TRoE

  apiEntityP = (emptyEntity == false)? kjClone(orionldState.kjsonP, orionldState.requestTree) : kjObject(orionldState.kjsonP, NULL);

  //
  // Entity id and type was removed - they need to go back
  // Here they are linked together   ( id -->  type )
  // They are here entered the tree of apiEntityP
  //
  orionldState.payloadIdNode->next   = orionldState.payloadTypeNode;
  orionldState.payloadTypeNode->next = (apiEntityP != NULL)? apiEntityP->value.firstChildP : NULL;


  //
  // An entity can be created without attributes.
  // Also, all attributes may be chopped off to exclusively registered endpoints.
  // The entity is still created in local
  //
  if (emptyEntity == true)
    apiEntityP->lastChild = orionldState.payloadTypeNode;

  apiEntityP->value.firstChildP = orionldState.payloadIdNode;

  //
  // The current shape of the incoming tree is now fit for TRoE, while it still needs to be adjusted for mongo,
  // If TRoE is enabled we clone it here, for later use in TRoE processing
  //
  // The same tree (read-only) might also be necessary for a 207 response in a distributed operation.
  // So, if anythis has been forwarded, the clone is made regardless whether TRoE is enabled.
  //
  if ((troe == true) || (distOpList != NULL))
    cloneForTroeP = kjClone(orionldState.kjsonP, apiEntityP);  // apiEntityP contains entity ID and TYPE

  if (orionldState.requestTree == NULL)
    orionldState.requestTree = kjObject(orionldState.kjsonP, NULL);
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
      goto awaitDoResponses;
  }

  dbEntityP = orionldState.requestTree;  // More adequate to talk about DB-Entity after the call to dbModelFromApiEntity

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
      distOpFailure(responseBody, NULL, "Database Error", "mongocEntityInsert failed", 500, NULL);
      goto awaitDoResponses;
    }
  }

  if (distOpList != NULL)  // NOT Purely local request
  {
    //
    // Need to call distOpSuccess here. Only, I don't have a DistOp object for local ...
    // Only the "DistOp::body" is used in distOpSuccess so I can fix it:
    //
    DistOp local;
    bzero(&local, sizeof(local));
    local.requestBody = cloneForTroeP;
    distOpSuccess(responseBody, &local, NULL, NULL);
  }

  //
  // Prepare for notifications
  //
  orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  orionldState.alterations->entityId                    = entityId;
  orionldState.alterations->entityType                  = entityType;
  orionldState.alterations->inEntityP                   = apiEntityP;
  orionldState.alterations->dbEntityP                   = NULL;
  orionldState.alterations->finalApiEntityP             = apiEntityP;
  orionldState.alterations->finalApiEntityWithSysAttrsP = kjClone(orionldState.kjsonP, apiEntityP);  // Later we add createdAt+modifiedAt
  orionldState.alterations->alteredAttributes           = 0;
  orionldState.alterations->alteredAttributeV           = NULL;
  orionldState.alterations->next                        = NULL;

  //
  // Must add the sysAttrs to the "Final API Entity" as subscriptions may have "notification::sysAttrs" set to true ...
  //
  sysAttrsToEntity(orionldState.alterations->finalApiEntityWithSysAttrsP);

  if (cloneForTroeP != NULL)
    orionldState.requestTree = cloneForTroeP;

  if (distOpList == NULL)  // Purely local request
  {
    orionldState.httpStatusCode = 201;
    orionldState.responseTree = NULL;
    httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);
    return true;
  }

 awaitDoResponses:
  if (distOpList != NULL)
    distOpResponses(distOpList, responseBody);

  responseFix(responseBody, DoCreateEntity, 201, entityId);

  if (orionldState.curlDoMultiP != NULL)
    distOpListRelease(distOpList);

  return true;
}
