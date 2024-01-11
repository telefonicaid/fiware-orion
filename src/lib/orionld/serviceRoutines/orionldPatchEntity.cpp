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
#include <strings.h>                                             // bzero

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/attributeUpdated.h"                     // attributeUpdated
#include "orionld/common/attributeNotUpdated.h"                  // attributeNotUpdated
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeType
#include "orionld/legacyDriver/legacyPatchEntity.h"              // legacyPatchEntity
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_OBJECT, PCHECK_URI, ...
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // dbModelFromApiAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocAttributesAdd.h"                  // mongocAttributesAdd
#include "orionld/distOp/distOpRequests.h"                       // distOpRequests
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/notifications/alteration.h"                    // alteration
#include "orionld/notifications/previousValuePopulate.h"         // previousValuePopulate
#include "orionld/notifications/sysAttrsStrip.h"                 // sysAttrsStrip
#include "orionld/kjTree/kjSort.h"                               // kjStringArraySort
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
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
    KjNode* oldAttrP    = kjLookup(finalApiEntityP, apiAttrP->name);
    KjNode* createdAtP  = NULL;
    KjNode* modifiedAtP = NULL;

    if (oldAttrP != NULL)
    {
      kjChildRemove(finalApiEntityP, oldAttrP);
      createdAtP  = kjLookup(oldAttrP, "createdAt");
      modifiedAtP = kjLookup(oldAttrP, "modifiedAt");
    }

    KjNode* attrCopy = kjClone(orionldState.kjsonP, apiAttrP);
    kjChildAdd(finalApiEntityP, attrCopy);

    if (createdAtP != NULL)
      kjChildAdd(attrCopy, createdAtP);
    if (modifiedAtP != NULL)
      kjChildAdd(attrCopy, modifiedAtP);
  }
}



// -----------------------------------------------------------------------------
//
// pCheckEntityType2 -
//
char* pCheckEntityType2(KjNode* payloadTypeNode, KjNode* dbEntityP, char* entityTypeFromUriParam)
{
  KjNode* dbTypeNodeP           = (dbEntityP != NULL)? dbEntityTypeExtract(dbEntityP) : NULL;
  char*   entityTypeFromDB      = (dbTypeNodeP != NULL)? dbTypeNodeP->value.s : NULL;
  char*   entityTypeFromPayload = (orionldState.payloadTypeNode != NULL)? orionldState.payloadTypeNode->value.s : NULL;

  LM_T(LmtSR, ("entityType From DB:        '%s'", entityTypeFromDB));
  LM_T(LmtSR, ("entityType From URI Param: '%s'", entityTypeFromUriParam));
  LM_T(LmtSR, ("entityType From Payload:   '%s'", entityTypeFromPayload));

  //
  // 3 different way to find the Entity Type:
  //   1. From the database
  //   2. From the URI Param "type"
  //   3. From the payload body
  //
  // The "truth" is in the database and that is always present.
  // - It could be an entity that lives elsewhere though ... then it won't be found in the DB, and that's OK
  //
  // Question is, what to do if URI Param "type" or payload body "type" differ (from DB "type" or between the two)
  // For now (as multi type isn't supported yet), I'll give error if mismatch
  //
  if ((entityTypeFromPayload != NULL) && (entityTypeFromDB != NULL))
  {
    if (strcmp(entityTypeFromDB, entityTypeFromPayload) != 0)
    {
      orionldError(OrionldBadRequestData, "Mismatching Entity::type in payload body", "Does not coincide with the Entity::type in the database", 400);
      LM_E(("Entity type in database:       '%s'", entityTypeFromDB));
      LM_E(("Entity type from payload body: '%s'", entityTypeFromPayload));
      return NULL;
    }
  }

  if ((entityTypeFromUriParam != NULL) && (entityTypeFromDB != NULL))
  {
    if (strcmp(entityTypeFromDB, entityTypeFromUriParam) != 0)
    {
      orionldError(OrionldBadRequestData, "Mismatching Entity type in URL parameter 'type'", "Does not coincide with the Entity::type in the database", 400);
      LM_E(("Entity type in database:       '%s'", entityTypeFromDB));
      LM_E(("Entity type from URI param:    '%s'", entityTypeFromUriParam));
      return NULL;
    }
  }

  if ((entityTypeFromPayload != NULL) && (entityTypeFromUriParam != NULL))
  {
    if (strcmp(entityTypeFromPayload, entityTypeFromUriParam) != 0)
    {
      orionldError(OrionldBadRequestData, "Mismatching Entity type in URL parameter 'type'", "Does not coincide with the Entity::type in payload body", 400);
      LM_E(("Entity type from URI param:    '%s'", entityTypeFromUriParam));
      LM_E(("Entity type from payload body: '%s'", entityTypeFromPayload));
      return NULL;
    }
  }

  if (entityTypeFromDB != NULL)
    return entityTypeFromDB;
  else if (entityTypeFromUriParam != NULL)
    return entityTypeFromUriParam;

  return entityTypeFromPayload;
}



// -----------------------------------------------------------------------------
//
// attributeLookup -
//
static bool attributeLookup(KjNode* dbAttrsP, char* attrName)
{
  dotForEq(attrName);

  KjNode* dbAttrP = kjLookup(dbAttrsP, attrName);
  if (dbAttrP == NULL)
    LM_T(0, ("Attribute '%s' does not exist locally", attrName));

  eqForDot(attrName);

  return (dbAttrP != NULL);
}



#if 0
// -----------------------------------------------------------------------------
//
// rawResponse -
//
void rawResponse(DistOp* distOpList, const char* what)
{
  LM_T(LmtSR, ("=============== rawResponse: %s", what));
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    if (distOpP->rawResponse != NULL)
      LM_T(LmtSR, ("%s: rawResponse: '%s'", distOpP->regP->regId, distOpP->rawResponse));
    else
      LM_T(LmtSR, ("%s: rawResponse: NULL", distOpP->regP->regId));
  }
  LM_T(LmtSR, ("===================================================================="));
}
#endif



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

  KjNode*            finalApiEntityWithSysAttrs = NULL;
  KjNode*            finalApiEntityP            = NULL;
  OrionldAlteration* alterationP                = NULL;
  KjNode*            incomingP                  = NULL;
  KjNode*            dbEntityCopy               = NULL;
  DistOp*            distOpList                 = NULL;
  char*              entityId                   = orionldState.wildcard[0];
  char*              entityType                 = orionldState.uriParams.type;
  KjNode*            attrP;
  KjNode*            next;

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
  KjNode* dbEntityP = mongocEntityLookup(entityId, entityType, NULL, NULL, NULL);
  if ((dbEntityP == NULL) && (orionldState.distributed == false))
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  //
  // If entity type is present in the payload body, it must be a String and identical to the entity type in the database.
  // [ It is already extracted (by mhdConnectionTreat) and checked for String ]
  //
  entityType = pCheckEntityType2(orionldState.payloadTypeNode, dbEntityP, entityType);

  KjNode* dbAttrsP = (dbEntityP != NULL)? kjLookup(dbEntityP, "attrs") : NULL;
  if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
  {
    LM_W(("Invalid payload body. %s: %s", orionldState.pd.title, orionldState.pd.detail));
    return false;
  }


  //
  // Distributed Operations
  //
  KjNode* responseBody = kjObject(orionldState.kjsonP, NULL);
  if (orionldState.distributed == true)
  {
    distOpList = distOpRequests(entityId, entityType, DoUpdateEntity, orionldState.requestTree);

    //
    // Read the responses from Distributed Requests
    //
    if (distOpList != NULL)
    {
      distOpResponses(distOpList, responseBody);
      distOpListRelease(distOpList);
    }
  }

  // Anything to actually PATCH locally? (or were all attributes Invalid|Forwarded [via Exclusive/Redirect registrations])?
  if ((orionldState.requestTree == NULL) || (orionldState.requestTree->value.firstChildP == NULL))
  {
    orionldState.requestTree = NULL;  // Nothing updated, nothing to be done for TRoE nor for Notifications
    goto done;
  }

  dbEntityCopy = kjClone(orionldState.kjsonP, dbEntityP);  // dbModelToApiEntity2 is DESTRUCTIVE

  //
  // Transform the remaining attributes to DB format, for storage in local database
  // But before that, as this is DESTRUCTIVE, save a copy of the attributes for Alterations and TRoE
  //
  incomingP = kjClone(orionldState.kjsonP, orionldState.requestTree);  // For Alterations and TRoE

  LM_T(LmtShowChanges, ("Looping over modified attributes"));

  attrP = orionldState.requestTree->value.firstChildP;
  while (attrP != NULL)
  {
    next = attrP->next;

    LM_T(LmtShowChanges, ("Modified attribute: '%s'", attrP->name));
    if (attributeLookup(dbAttrsP, attrP->name) == false)
    {
      LM_T(LmtSR, ("Removing attribute '%s' from the incoming tree", attrP->name));
      kjChildRemove(orionldState.requestTree, attrP);
      distOpFailure(responseBody, NULL, "Attribute Not Found", NULL, 404, attrP->name);

      // Must remove it from the cloned incomingP as well (can't be used for Alterations and TRoE)
      attrP = kjLookup(incomingP, attrP->name);
      if (attrP != NULL)
        kjChildRemove(incomingP, attrP);
    }
    else
    {
      LM_T(LmtShowChanges, ("Lookup attribute '%s' in dbAttrsP and copy its value - store in orionldState", attrP->name));
      char* eqName = kaStrdup(&orionldState.kalloc, attrP->name);
      dotForEq(eqName);

      previousValuePopulate(dbAttrsP, NULL, eqName);
      dbModelFromApiAttribute(attrP, dbAttrsP, NULL, NULL, NULL, true);  // Removes creDate for attributes
    }

    attrP = next;
  }

  //
  // If no attributes left in orionldState.requestTree, we're done
  // This happens if all attributes were either faulty or forwarded via Exclusive/Redirect registrations.
  //
  if ((orionldState.requestTree == NULL) || (orionldState.requestTree->value.firstChildP == NULL))
    goto done;

  if (mongocAttributesAdd(entityId, NULL, orionldState.requestTree, false) == false)
  {
    if (distOpList == NULL)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocAttributesAdd failed", 500);
      return false;
    }

    //
    // FIXME: Can't return here ... what about distOps?
    //
    // Instead - add all attrs from orionldState.requestTree to notUpdatedV, unless already found in updatedV
    // + if present in notUpdatedV, remove that slot (replace it by a 500 slot, which is more important)
    //
    for (KjNode* attrP = incomingP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      distOpFailure(responseBody, NULL, "Database Error", "mongocAttributesAdd failed", 500, attrP->name);
    }
  }
  else  // All OK - need to add the successfully added/updated attributes with "distOpSuccess"
  {
    // Only, I need a DistOp for that ...
    // All that is needed is the body, sop, we can create a "fake" DistOp:
    //
    DistOp local;

    bzero(&local, sizeof(local));
    local.requestBody = orionldState.requestTree;
    distOpSuccess(responseBody, &local, entityId, NULL);
  }

  //
  // For alteration() we need:
  // - Initial DB entity, before the merge  (dbEntityP)
  // - Incoming Entity, expanded and normalized, for watchedAttributes  (incomingP)
  // - Final API Entity, for q, etc AND for the Notifications (finalApiEntityP)
  //     Note; the Final API Entity needs to be Expanded and Normalized - each subscription may have a different context to for compaction
  //


  finalApiEntityWithSysAttrs = dbModelToApiEntity2(dbEntityCopy, true, RF_NORMALIZED, NULL, false, &orionldState.pd);
  if (finalApiEntityWithSysAttrs == NULL)
  {
    // There will be no notifications for this update :(  - Well.  cause something important failed ...
    LM_E(("dbModelToApiEntity2: %s: %s", orionldState.pd.title, orionldState.pd.detail));
    goto done;
  }


  //
  // Notifications
  //
  attributesMerge(finalApiEntityWithSysAttrs, incomingP);

  finalApiEntityP = kjClone(orionldState.kjsonP, finalApiEntityWithSysAttrs);
  sysAttrsStrip(finalApiEntityP);

  alterationP = alteration(entityId, entityType, finalApiEntityP, incomingP, dbEntityP);
  alterationP->finalApiEntityWithSysAttrsP = finalApiEntityWithSysAttrs;

  //
  // For TRoE we need:
  // - Incoming Entity, normalized
  //
  orionldState.requestTree = incomingP;

 done:
  responseFix(responseBody, DoUpdateEntity, 204, entityId);

  return true;
}
