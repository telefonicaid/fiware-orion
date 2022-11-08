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
#include <curl/curl.h>                                           // curl

#include "logMsg/logMsg.h"                                       // LM_*

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

#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/performance.h"                          // PERFORMANCE
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/types/RegistrationMode.h"                      // registrationMode
#include "orionld/legacyDriver/legacyPostEntities.h"             // legacyPostEntities
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pCheckEntityId.h"                 // pCheckEntityId
#include "orionld/payloadCheck/pCheckEntityType.h"               // pCheckEntityType
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/dbModel/dbModelFromApiEntity.h"                // dbModelFromApiEntity
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityInsert.h"                   // mongocEntityInsert
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/forwardRequestSend.h"               // forwardRequestSend
#include "orionld/forwarding/forwardingRelease.h"                // forwardingRelease
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



char userAgentHeaderNoLF[64];     // "User-Agent: orionld/" + ORIONLD_VERSION - initialized in orionldServiceInit()
char hostHeader[256];


void forwardingSuccess(KjNode* responseBody, ForwardPending* fwdPendingP)
{
  KjNode* successV = kjLookup(responseBody, "success");

  if (successV == NULL)
  {
    successV = kjArray(orionldState.kjsonP, "success");
    kjChildAdd(responseBody, successV);
  }

  for (KjNode* attrNameP = fwdPendingP->body->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
  {
    if (strcmp(attrNameP->name, "id") == 0)
      continue;
    if (strcmp(attrNameP->name, "type") == 0)
      continue;

    char*   alias  = orionldContextItemAliasLookup(orionldState.contextP, attrNameP->name, NULL, NULL);
    KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);
    kjChildAdd(successV, aNameP);
  }
}


//
// 207 response for Entity Creation:
//
// {
//   "entityId": "xxx",
//   "success": [
//     {
//       "attributes": [ "", ""],
//     }
//   ],
//   "failure": [
//     {
//       "attributes": [ "", ""],
//       "statusCode": 404,
//       "title: "xxx",
//       "detail: "yyy"
//     }
// }
//
void forwardingFailure(KjNode* responseBody, ForwardPending* fwdPendingP, OrionldResponseErrorType errorCode, const char* title, const char* detail, int httpStatus)
{
  KjNode* failureV = kjLookup(responseBody, "failure");

  if (failureV == NULL)
  {
    failureV = kjArray(orionldState.kjsonP, "failure");
    kjChildAdd(responseBody, failureV);
  }

  KjNode* regIdP      = (fwdPendingP != NULL)? kjLookup(fwdPendingP->regP->regTree, "id") : NULL;
  KjNode* regIdNodeP  = (regIdP != NULL)? kjString(orionldState.kjsonP, "registrationId", regIdP->value.s) : NULL;
  KjNode* error       = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrV       = kjArray(orionldState.kjsonP, "attributes");
  KjNode* statusCodeP = kjInteger(orionldState.kjsonP, "statusCode", httpStatus);
  KjNode* titleP      = kjString(orionldState.kjsonP, "title", title);
  KjNode* detailP     = kjString(orionldState.kjsonP, "detail", detail);

  if (regIdNodeP != NULL)
    kjChildAdd(error, regIdNodeP);

  kjChildAdd(error, attrV);
  kjChildAdd(error, statusCodeP);
  kjChildAdd(error, titleP);
  kjChildAdd(error, detailP);

  for (KjNode* attrNameP = fwdPendingP->body->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
  {
    if (strcmp(attrNameP->name, "id") == 0)
      continue;
    if (strcmp(attrNameP->name, "type") == 0)
      continue;

    char*   alias  = orionldContextItemAliasLookup(orionldState.contextP, attrNameP->name, NULL, NULL);
    KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);
    kjChildAdd(attrV, aNameP);
  }

  kjChildAdd(failureV, error);
}



// -----------------------------------------------------------------------------
//
// fwdPendingLookupByCurlHandle -
//
ForwardPending* fwdPendingLookupByCurlHandle(ForwardPending* fwdPendingList, CURL* easyHandle)
{
  ForwardPending* fwdPendingP = fwdPendingList;

  while (fwdPendingP != NULL)
  {
    if (fwdPendingP->curlHandle == easyHandle)
      return fwdPendingP;

    fwdPendingP = fwdPendingP->next;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// Matching with "information"
//
// "information": [
//   {
//     "entities": [
//       {
//         "id": "urn:E1",
//         "idPattern": "xxx",   # Can't be present in an exclusive registration
//         "type": "T"
//       }
//     ],
//     "propertyNames": [ "P1", "P2" ]
//     "relationshipNames": [ "R1", "R2" ]
//   }
// ]
//
//
bool regMatchEntityInfo(KjNode* entityInfoP, const char* entityId, const char* entityType)
{
  KjNode* idP         = kjLookup(entityInfoP, "id");
  // KjNode* idPatternP  = kjLookup(entityInfoP, "idPattern");
  KjNode* typeP       = kjLookup(entityInfoP, "type");

  //
  // "type" is mandatory for all 'entityInfo'
  // "id" is mandatory for 'exclusive' registrations
  //
  //
  if (typeP == NULL)
  {
    LM(("RM: No match due to invalid registration (no type in EntityInfo)"));
    return false;
  }

  if (strcmp(typeP->value.s, entityType) != 0)
  {
    LM(("RM: No match due to entity type ('%s' in reg, '%s' in entity creation)", typeP->value.s, entityType));
    return false;
  }

  if (idP != NULL)
  {
    if (strcmp(idP->value.s, entityId) != 0)
    {
      LM(("RM: No match due to entity id ('%s' in reg, '%s' in entity creation)", idP->value.s, entityId));
      return false;
    }
  }

  // FIXME: idPattern

  return true;
}



// -----------------------------------------------------------------------------
//
// regMatchAttributes -
//
KjNode* regMatchAttributes(RegCacheItem* regP, KjNode* propertyNamesP, KjNode* relationshipNamesP, KjNode* incomingP)
{
  KjNode* attrObject = NULL;

  //
  // Registration of entire entity?
  // Only 'inclusive' registrations can do that
  //
  if ((propertyNamesP == NULL) && (relationshipNamesP == NULL))
  {
    KjNode* body = kjClone(orionldState.kjsonP, incomingP);

    // Add entity type and id
    KjNode* idP   = kjClone(orionldState.kjsonP, orionldState.payloadIdNode);
    KjNode* typeP = kjClone(orionldState.kjsonP, orionldState.payloadTypeNode);

    kjChildAdd(body, idP);
    kjChildAdd(body, typeP);

    return body;
  }

  KjNode* attrP = incomingP->value.firstChildP;
  KjNode* next;
  while (attrP != NULL)
  {
    next = attrP->next;

    KjNode* matchP = NULL;

    if (propertyNamesP != NULL)
      matchP = kjStringValueLookupInArray(propertyNamesP, attrP->name);
    if ((matchP == NULL) && (relationshipNamesP != NULL))
      matchP = kjStringValueLookupInArray(relationshipNamesP, attrP->name);

    if (matchP == NULL)
    {
      attrP = next;
      continue;
    }

    //
    // 'inclusive' registrations must CLONE the attribute
    // The other two (exclusive, redirect) STEAL the attribute
    //
    if (regP->mode == RegModeInclusive)
      matchP = kjClone(orionldState.kjsonP, attrP);
    else
    {
      matchP = attrP;
      kjChildRemove(incomingP, attrP);
    }

    if (attrObject == NULL)
      attrObject = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(attrObject, matchP);

    attrP = next;
  }

  if (attrObject == NULL)
    LM(("RM: No match due to no matching attributes"));

  return attrObject;
}



// -----------------------------------------------------------------------------
//
// regMatchInformationItem -
//
KjNode* regMatchInformationItem(RegCacheItem* regP, KjNode* infoP, const char* entityId, const char* entityType, KjNode* incomingP)
{
  KjNode* entities = kjLookup(infoP, "entities");

  if (entities != NULL)
  {
    bool match = false;
    for (KjNode* entityInfoP = entities->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      if (regMatchEntityInfo(entityInfoP, entityId, entityType) == true)
      {
        match = true;
        break;
      }
    }

    if (match == false)
      return NULL;
  }

  KjNode* propertyNamesP     = kjLookup(infoP, "propertyNames");
  KjNode* relationshipNamesP = kjLookup(infoP, "relationshipNames");
  KjNode* attrUnionP         = regMatchAttributes(regP, propertyNamesP, relationshipNamesP, incomingP);

  return attrUnionP;
}



// -----------------------------------------------------------------------------
//
// regMatchInformationArray -
//
ForwardPending* regMatchInformationArray(RegCacheItem* regP, const char* entityId, const char* entityType, KjNode* incomingP)
{
  KjNode* informationV = kjLookup(regP->regTree, "information");

  for (KjNode* infoP = informationV->value.firstChildP; infoP != NULL; infoP = infoP->next)
  {
    KjNode* attrUnion = regMatchInformationItem(regP, infoP, entityId, entityType, incomingP);

    if (attrUnion == NULL)
      continue;

    // If we get this far, then it's a match
    KjNode* entityIdP   = kjString(orionldState.kjsonP,  "id",   entityId);
    KjNode* entityTypeP = kjString(orionldState.kjsonP,  "type", entityType);

    kjChildAdd(attrUnion, entityIdP);
    kjChildAdd(attrUnion, entityTypeP);

    ForwardPending* fwdPendingP    = (ForwardPending*) kaAlloc(&orionldState.kalloc, sizeof(ForwardPending));

    fwdPendingP->regP = regP;
    fwdPendingP->body = attrUnion;

    return fwdPendingP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// regMatchOperation -
//
// FIXME: the operations should be a bitmask in RegCacheItem - no kjLookup, no string comparisons
//
bool regMatchOperation(RegCacheItem* regP, const char* op)
{
  return true;
}



// -----------------------------------------------------------------------------
//
// regMatchForEntityCreation -
//
// To match for Entity Creation, a registration needs:
// - "mode" != "auxiliary"
// - "operations" must include "createEntity
// - "information" must match by entity id+type and attributes if present in the registration
//
ForwardPending* regMatchForEntityCreation(const char* entityId, const char* entityType, KjNode* incomingP)
{
  ForwardPending* fwdPendingHead = NULL;
  ForwardPending* fwdPendingTail = NULL;

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    KjNode* regModeP = kjLookup(regP->regTree, "mode");  // FIXME: mode needs to be part of RegCacheItem (as an enum)

    // FIXME: Set the regP->mode at creation/update time
    regP->mode = (regModeP != NULL)? registrationMode(regModeP->value.s) : RegModeInclusive;
    if (regP->mode == RegModeAuxiliary)
      continue;

    if (regMatchOperation(regP, "createEntity") == false)  // FIXME: "createEntity" should be an enum value
      continue;

    ForwardPending* fwdPendingP = regMatchInformationArray(regP, entityId, entityType, incomingP);
    if (fwdPendingP == NULL)
      continue;

    // Add fwdPendingP to the linked list
    if (fwdPendingHead == NULL)
      fwdPendingHead = fwdPendingP;
    else
      fwdPendingTail->next = fwdPendingP;

    fwdPendingTail       = fwdPendingP;
    fwdPendingTail->next = NULL;
  }

  return fwdPendingHead;
}



// -----------------------------------------------------------------------------
//
// curlToBrokerStrerror -
//
const char* curlToBrokerStrerror(CURL* curlHandle, int curlErrorCode, int* statusCodeP)
{
  if (curlErrorCode == 5)
  {
    *statusCodeP = 504;
    return "Unable to resolve proxy";
  }
  else if (curlErrorCode == 6)
  {
    *statusCodeP = 504;
    return "Unable to resolve host name of registrant";
  }
  else if (curlErrorCode == 7)
  {
    *statusCodeP = 504;
    return "Unable to connect to registrant";
  }
  else if (curlErrorCode == 22)
  {
    long httpStatus;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpStatus);

    *statusCodeP = httpStatus;
    if (httpStatus == 409)
      return "Entity already exists";
    else
    {
      LM_W(("Forwarded request response is of HTTP Status %d", httpStatus));
      return "Entity was not created externally, and it did not previously exist";
    }
  }
  else
  {
    *statusCodeP = 500;
    return "Other CURL Error";
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

  int              forwards       = 0;
  ForwardPending*  fwdPendingList = NULL;
  KjNode*          responseBody   = NULL;

  if (forwarding)
  {
    KjNode* entityIdNodeP = kjString(orionldState.kjsonP, "entityId", entityId);  // Only used if 207 response

    responseBody = kjObject(orionldState.kjsonP, NULL);                           // Only used if 207 response
    kjChildAdd(responseBody, entityIdNodeP);

    char dateHeader[70];
    snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

    fwdPendingList = regMatchForEntityCreation(entityId, entityType, orionldState.requestTree);

    for (ForwardPending* fwdPendingP = fwdPendingList; fwdPendingP != NULL; fwdPendingP = fwdPendingP->next)
    {
      // Send the forwarded request and await all responses
      if (fwdPendingP->regP != NULL)
      {
        KjNode* endpointP = kjLookup(fwdPendingP->regP->regTree, "endpoint");
        LM(("RM: Forwarding to %s", endpointP->value.s));
        kjTreeLog(fwdPendingP->body, "RM: body to be forwarded");

        if (forwardRequestSend(fwdPendingP, dateHeader) == 0)
        {
          ++forwards;
          fwdPendingP->error = false;
        }
        else
          fwdPendingP->error = true;
      }
      else
        LM(("RM: regP == NULL - it's the local stuff"));
    }

    int stillRunning = 1;
    int loops        = 0;

    while (stillRunning != 0)
    {
      LM(("Calling curl_multi_perform"));
      CURLMcode cm = curl_multi_perform(orionldState.curlFwdMultiP, &stillRunning);
      if (cm != 0)
      {
        LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
        forwards = 0;
        break;
      }
      LM(("curl_multi_perform OK"));

      if (stillRunning != 0)
      {
        cm = curl_multi_wait(orionldState.curlFwdMultiP, NULL, 0, 1000, NULL);
        if (cm != CURLM_OK)
        {
          LM_E(("Internal Error (curl_multi_wait: error %d", cm));
          break;
        }
      }

      if ((++loops >= 10) && ((loops % 5) == 0))
        LM_W(("curl_multi_perform doesn't seem to finish ..."));
    }

    // Anything left for a local entity?
    if (orionldState.requestTree->value.firstChildP != NULL)
    {
      KjNode* entityIdP   = kjString(orionldState.kjsonP,  "id",   entityId);
      KjNode* entityTypeP = kjString(orionldState.kjsonP,  "type", entityType);

      kjChildAdd(orionldState.requestTree, entityIdP);
      kjChildAdd(orionldState.requestTree, entityTypeP);

      kjTreeLog(orionldState.requestTree, "RM: Left of incoming entity (for local creation)");
    }
    else
      orionldState.requestTree = NULL;  // Meaning: nothing left for local DB

    //
    // After sending all forwarded requests, we let the local DB access run
    // After that is done (if anything left for local DB),
    //   we select on the reponses and build the answer
    //
  }


  //
  // If the entity already exists, a "409 Conflict" is returned, either complete or as part of a 207
  //
  if (orionldState.requestTree != NULL)
  {
    if (mongocEntityLookup(entityId) != NULL)
    {
      if (fwdPendingList == NULL)  // Purely local request
      {
        orionldError(OrionldAlreadyExists, "Entity already exists", entityId, 409);
        return false;
      }
      else
      {
        forwardingFailure(responseBody, NULL, OrionldAlreadyExists, "Entity already exists", entityId, 404);
        goto awaitFwdResponses;
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
    if ((troe == true) || (fwdPendingList != NULL))
      cloneForTroeP = kjClone(orionldState.kjsonP, apiEntityP);  // apiEntityP contains entity ID and TYPE

    if (dbModelFromApiEntity(orionldState.requestTree, NULL, true, orionldState.payloadIdNode->value.s, orionldState.payloadTypeNode->value.s) == false)
    {
      //
      // Not calling orionldError as a better error message is overwritten if I do.
      // Once we have "Error Stacking", orionldError should be called.
      //
      // orionldError(OrionldInternalError, "Internal Error", "Unable to convert API Entity into DB Model Entity", 500);

      if (fwdPendingList == NULL)  // Purely local request
        return false;
      else
      {
        forwardingFailure(responseBody, NULL, OrionldInternalError, "Internal Error", "dbModelFromApiEntity failed", 500);
        goto awaitFwdResponses;
      }
    }

    KjNode* dbEntityP = orionldState.requestTree;  // More adecuate to talk about DB-Entity from here on

    // datasets?
    if (orionldState.datasets != NULL)
      kjChildAdd(dbEntityP, orionldState.datasets);

    // Ready to send it to the database
    if (mongocEntityInsert(dbEntityP, entityId) == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntityInsert failed", 500);
      if (fwdPendingList == NULL)  // Purely local request
        return false;
      else
      {
        forwardingFailure(responseBody, NULL, OrionldInternalError, "Database Error", "mongocEntityInsert failed", 500);
        goto awaitFwdResponses;
      }
    }
    else if (fwdPendingList != NULL)  // NOT Purely local request
    {
      //
      // Need to call forwardingSuccess here. Only, I don't have a ForwardPending object for local ...
      // Only the "ForwardPending::body" is used in forwardingSuccess so I can fix it:
      //
      ForwardPending local;
      local.body = cloneForTroeP;
      forwardingSuccess(responseBody, &local);
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

    if (fwdPendingList == NULL)  // Purely local request
      return true;
  }

 awaitFwdResponses:
  if ((forwarding) && (forwards > 0))
  {
    CURLMsg* msgP;
    int      msgsLeft;

    LM(("Reading the responses of the forwarded requests"));
    while ((msgP = curl_multi_info_read(orionldState.curlFwdMultiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      ForwardPending* fwdPendingP = fwdPendingLookupByCurlHandle(fwdPendingList, msgP->easy_handle);

      if (msgP->data.result == CURLE_OK)
        forwardingSuccess(responseBody, fwdPendingP);
      else
      {
        int          statusCode = 500;
        const char*  detail     = curlToBrokerStrerror(msgP->easy_handle, msgP->data.result, &statusCode);

        LM_E(("CURL Error %d: %s", msgP->data.result, curl_easy_strerror(msgP->data.result)));
        forwardingFailure(responseBody, fwdPendingP, OrionldInternalError, "Error during Forwarding", detail, statusCode);
      }
    }
  }

  if ((responseBody != NULL) && (kjLookup(responseBody, "failure") != NULL))
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

  if (orionldState.curlFwdMultiP != NULL)
    forwardingRelease(fwdPendingList);
  return true;
}
