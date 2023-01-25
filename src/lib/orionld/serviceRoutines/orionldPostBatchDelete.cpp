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
#include "kjson/kjBuilder.h"                                   // kjArray, kjObject, kjNull, ...
#include "kjson/kjRender.h"                                    // kjFastRender
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
#include "kjson/kjParse.h"                                     // kjParse
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
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/mongoc/mongocEntitiesExist.h"                // mongocEntitiesExist
#include "orionld/mongoc/mongocEntitiesDelete.h"               // mongocEntitiesDelete
#include "orionld/forwarding/ForwardPending.h"                 // ForwardPending
#include "orionld/forwarding/forwardingListsMerge.h"           // forwardingListsMerge
#include "orionld/forwarding/forwardRequestSend.h"             // forwardRequestSend
#include "orionld/forwarding/fwdPendingLookupByCurlHandle.h"   // fwdPendingLookupByCurlHandle
#include "orionld/forwarding/xForwardedForCompose.h"           // xForwardedForCompose
#include "orionld/forwarding/regMatchForBatchDelete.h"         // regMatchForBatchDelete
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



// -----------------------------------------------------------------------------
//
// dbModelToEntityIdAndTypeTable -
//
// - In (dbEntityIdArray) comes DB format array of _id:
//   [
//     {
//       "_id": {
//         "id": "urn:ngsi-ld:entity:E1",
//         "type": "https://uri.etsi.org/ngsi-ld/default-context/T"
//       }
//     },
//     ...
//   ]
//
// - Out goes an object:   { "id1": "type1", "id2": "type2", ... }
//
KjNode* dbModelToEntityIdAndTypeTable(KjNode* dbEntityIdArray)
{
  KjNode* outP = kjObject(orionldState.kjsonP, NULL);

  for (KjNode* arrayItem = dbEntityIdArray->value.firstChildP; arrayItem != NULL; arrayItem = arrayItem->next)
  {
    KjNode* _idNodeP = kjLookup(arrayItem, "_id");
    if (_idNodeP == NULL)
      continue;

    KjNode* idP      = kjLookup(_idNodeP, "id");
    KjNode* typeP    = kjLookup(_idNodeP, "type");
    if ((idP == NULL) || (typeP == NULL))
      continue;

    KjNode* idAndTypeNodeP = kjString(orionldState.kjsonP, idP->value.s, typeP->value.s);

    kjChildAdd(outP, idAndTypeNodeP);
  }

  return outP;
}



// ----------------------------------------------------------------------------
//
// distReqLog -
//
void distReqLog(ForwardPending* distReqList, const char* title)
{
  LM(("DR: %s", title));
  for (ForwardPending* drP = distReqList; drP != NULL; drP = drP->next)
  {
    LM(("DR: Registration:               %s", drP->regP->regId));
    LM(("DR: Verb:                       %s", "TBD"));
    LM(("DR: URL:                        %s", "TBD"));  // IP+port should be in the URL
    LM(("DR: Entity ID:                  %s", drP->entityId));
    LM(("DR: Entity Type:                %s", drP->entityType));

    if (drP->body != NULL)
    {
      int   bodySize = kjFastRenderSize(drP->body);
      char* body     = kaAlloc(&orionldState.kalloc, bodySize + 256);
      kjFastRender(drP->body, body);
      LM(("DR: payload body:               %s", body));
    }
    else
      LM(("DR: payload body:               NULL"));

    LM(("DR: -----------------------------------------------------------"));
  }
}



// -----------------------------------------------------------------------------
//
// firstItemOfRegLookup -
//
ForwardPending* firstItemOfRegLookup(ForwardPending* distReqList, RegCacheItem* regP)
{
  for (ForwardPending* drP = distReqList; drP != NULL; drP = drP->next)
  {
    if (drP->regP == regP)
      return drP;
  }

  return NULL;  // Can never happen
}



// -----------------------------------------------------------------------------
//
// distReqsMergeForBatchDelete -
//
void distReqsMergeForBatchDelete(ForwardPending* distReqList)
{
  //
  // Payload body needed for the entity id(s)
  //
  ForwardPending* drP  = distReqList;
  ForwardPending* next;
  ForwardPending* prev = NULL;

  while (drP != NULL)
  {
    next = drP->next;

    //
    // Either it's the first, in which case the drP->body needs to be created (as an array and entityId added to it)
    // Or, the first must be found, and the entityId added to its array (and the drP item to be removed)
    //
    ForwardPending* firstP = firstItemOfRegLookup(distReqList, drP->regP);

    if (drP == firstP)
    {
      LM(("Keeping drP  { reg: %s, entityId: %s } - creating BODY", drP->regP->regId, drP->entityId));
      drP->body = kjArray(orionldState.kjsonP, NULL);

      prev = drP;  // Only setting 'prev' here as in the "esle case", drP is skipped
    }
    else
    {
      // Remove drP from list (just skip over it
      prev->next = next;
      LM(("Removing drP { reg: %s, entityId: %s } from the list", drP->regP->regId, drP->entityId));
    }

    // Now add the entityId to the array of "firstP"
    KjNode* eidNodeP = kjString(orionldState.kjsonP, NULL, drP->entityId);
    kjChildAdd(firstP->body, eidNodeP);

    drP  = next;
  }
}



// -----------------------------------------------------------------------------
//
// entityIdAndTypeTableToIdArray -
//
KjNode* entityIdAndTypeTableToIdArray(KjNode* entityIdAndTypeTable)
{
  KjNode* outArray = kjArray(orionldState.kjsonP, "success");   // Needs to be called success later so ... why not here already ...

  for (KjNode* eidNodeP = entityIdAndTypeTable->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
  {
    KjNode* eidP = kjString(orionldState.kjsonP, NULL, eidNodeP->name);
    kjChildAdd(outArray, eidP);
  }

  return outArray;
}


// -----------------------------------------------------------------------------
//
// entityIdLookupInObjectArray -
//
static KjNode* entityIdLookupInObjectArray(KjNode* objectArray, char* entityId)
{
  for (KjNode* arrayItemP = objectArray->value.firstChildP; arrayItemP != NULL; arrayItemP = arrayItemP->next)
  {
    KjNode* itemP = kjLookup(arrayItemP, "entityId");

    if ((itemP != NULL) && (itemP->type == KjString))
    {
      if (strcmp(itemP->value.s, entityId) == 0)
        return arrayItemP;
    }
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// responseMerge -
//
static void responseMerge(ForwardPending* drP, KjNode* responseSuccess, KjNode* responseErrors)
{
  kjTreeLog(drP->body, "KZ: drP->body BEFORE");
  kjTreeLog(responseSuccess, "KZ: responseSuccess BEFORE");
  kjTreeLog(responseErrors, "KZ: responseErrors BEFORE");
  LM(("KZ: drP->httpResponseCode == %d", drP->httpResponseCode));

  if (drP->httpResponseCode == 204)
  {
    LM(("KZ: 204 - move all drP->body entity ids from drP to responseSuccess"));
    LM(("KZ: 204 - remove all drP->body entity ids from responseErrors"));

    for (KjNode* eidNodeP = drP->body->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
    {
      KjNode* nodeP;

      // 1. Remove from responseErrors
      nodeP = entityIdLookupInObjectArray(responseErrors, eidNodeP->value.s);
      if (nodeP != NULL)
        kjChildRemove(responseErrors, nodeP);

      // 2. Add to responseSuccess
      nodeP = kjStringValueLookupInArray(responseSuccess, eidNodeP->value.s);
      if (nodeP == NULL)
      {
        KjNode* eidP = kjString(orionldState.kjsonP, NULL, eidNodeP->value.s);
        kjChildAdd(responseSuccess, eidP);
      }
    }
  }
  else if (drP->httpResponseCode == 207)
  {
    LM(("KZ: 207 - move all drP->responseBody::success entity ids from drP to responseSuccess"));
    LM(("KZ: 207 - move all drP->responseBody::errors entity ids from drP to responseErrors"));

    KjNode* successV = kjLookup(drP->responseBody, "success");
    KjNode* errorsV  = kjLookup(drP->responseBody, "errors");

    if (successV != NULL)
    {
      for (KjNode* eidNodeP = successV->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
      {
        // 1. Remove from responseErrors
        KjNode* nodeP = entityIdLookupInObjectArray(responseErrors, eidNodeP->value.s);
        if (nodeP != NULL)
          kjChildRemove(responseErrors, nodeP);

        // 2. Add to responseSuccess
        nodeP = kjStringValueLookupInArray(responseSuccess, eidNodeP->value.s);
        if (nodeP == NULL)
        {
          KjNode* eidP = kjString(orionldState.kjsonP, NULL, eidNodeP->value.s);
          kjChildAdd(responseSuccess, eidP);
        }
      }
    }

    if (errorsV != NULL)
    {
      //
      // For errors without Registration ID, add it !
      //
      for (KjNode* errorBodyP = errorsV->value.firstChildP; errorBodyP != NULL; errorBodyP = errorBodyP->next)
      {
        KjNode* eidNodeP = kjLookup(errorBodyP, "entityId");

        if (eidNodeP != NULL)
        {
          KjNode* oldErrorItemP = entityIdLookupInObjectArray(responseErrors, eidNodeP->value.s);

          if (oldErrorItemP == NULL)
          {
            LM(("KZ: Entity ID '%s' is added to responseErrors (unless it's already in the successArray)", eidNodeP->value.s));

            if (kjStringValueLookupInArray(responseSuccess, eidNodeP->value.s) == NULL)
            {
              kjChildRemove(errorsV, errorBodyP);
              kjChildAdd(responseErrors, errorBodyP);

              KjNode* registrationIdNodeP = kjLookup(errorBodyP, "registrationId");
              if (registrationIdNodeP == NULL)
              {
                registrationIdNodeP = kjString(orionldState.kjsonP, "registrationId", drP->regP->regId);
                kjChildAdd(errorBodyP, registrationIdNodeP);
              }
            }
          }
        }
      }
    }
  }
  else
  {
    LM(("%d - like 204 but to responseErrors instead of responseSuccess ...", drP->httpResponseCode));
  }

  kjTreeLog(drP->body, "KZ: drP->body AFTER");
  kjTreeLog(responseSuccess, "KZ: responseSuccess AFTER");
  kjTreeLog(responseErrors, "KZ: responseErrors AFTER");
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

  KjNode* responseErrors  = kjArray(orionldState.kjsonP, "errors");
  KjNode* responseSuccess = kjArray(orionldState.kjsonP, "success");

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

  // Now we have an Array with valid Entity IDs - time to ask mongo if they actually exist (locally)
  kjTreeLog(orionldState.requestTree, "Entities to DELETE");
  KjNode* dbEntityIdArray = mongocEntitiesExist(orionldState.requestTree, true);

  // Simplify the DB _id array to an KV object   "entityId": "entityType "
  kjTreeLog(dbEntityIdArray, "Existing Entities from DB");

  KjNode* entityIdAndTypeTable = NULL;
  bool   dbOperationOk         = false;

  if ((dbEntityIdArray == NULL) || (dbEntityIdArray->value.firstChildP == NULL))
  {
    entityIdAndTypeTable = kjObject(orionldState.kjsonP,  NULL);
  }
  else
  {
    entityIdAndTypeTable = dbModelToEntityIdAndTypeTable(dbEntityIdArray);
    kjTreeLog(entityIdAndTypeTable, "entityIdAndTypeTable");

    responseSuccess = entityIdAndTypeTableToIdArray(entityIdAndTypeTable);
    dbOperationOk = mongocEntitiesDelete(responseSuccess);
    if (dbOperationOk == false)
    {
      LM(("KZ3: mongocEntitiesDelete returned FALSE !!!   -  Treat it as a 404 for all entities"));
      responseErrors  = responseSuccess;
      responseSuccess = kjArray(orionldState.kjsonP, "success");   // EMPTY ARRAY
      // FIXME: Remodel responseErrors to follow the API
    }
  }

  //
  // Those Entity IDs that did not exist locally are set as 404 in responseErrors
  // This might change, if distributed operations, but for now they're 404s
  //
  for (KjNode* eidP = orionldState.requestTree->value.firstChildP; eidP != NULL; eidP = eidP->next)
  {
    if (kjStringValueLookupInArray(responseSuccess, eidP->value.s) == NULL)
    {
      LM(("KZ3: Adding entity '%s' to responseErrors", eidP->value.s));
      KjNode* objectP       = kjObject(orionldState.kjsonP,  NULL);
      KjNode* eidNodeP      = kjString(orionldState.kjsonP,  "entityId", eidP->value.s);
      KjNode* pdNodeP       = kjObject(orionldState.kjsonP,  "error");
      KjNode* pdTitleNodeP  = kjString(orionldState.kjsonP,  "title", "Entity Not Found");
      KjNode* pdDetailNodeP = kjString(orionldState.kjsonP,  "detail", "Cannot delete entities that do not exist");
      KjNode* pdStatusNodeP = kjInteger(orionldState.kjsonP, "status", 404);
      KjNode* pdTypeNodeP   = kjString(orionldState.kjsonP,  "type", orionldResponseErrorType(OrionldResourceNotFound));

      kjChildAdd(pdNodeP, pdTitleNodeP);
      kjChildAdd(pdNodeP, pdDetailNodeP);
      kjChildAdd(pdNodeP, pdTypeNodeP);
      kjChildAdd(pdNodeP, pdStatusNodeP);

      kjChildAdd(objectP, eidNodeP);
      kjChildAdd(objectP, pdNodeP);

      kjChildAdd(responseErrors, objectP);
    }
    else
      LM(("KZ3: NOT Adding entity '%s' to responseErrors (cause found in responseSuccess)", eidP->value.s));
  }

  kjTreeLog(responseSuccess, "Initial SUCCESS Array");
  kjTreeLog(responseErrors, "Initial ERRORS Array");

  // Add any entity ids that were not found in the DB to entityIdAndTypeTable as "entityId": null, now that the entity type is unknown
  for (KjNode* inEntityIdNodeP = orionldState.requestTree->value.firstChildP; inEntityIdNodeP != NULL; inEntityIdNodeP = inEntityIdNodeP->next)
  {
    char* entityId = inEntityIdNodeP->value.s;
    if (kjLookup(entityIdAndTypeTable, entityId) == NULL)
    {
      KjNode* nullNodeP = kjNull(orionldState.kjsonP, entityId);
      kjChildAdd(entityIdAndTypeTable, nullNodeP);
    }
  }
  kjTreeLog(entityIdAndTypeTable, "entityIdAndTypeTable (with unfound entities added)");


  // Get the list of Forwarded requests, for matching registrations
  bool distributed = (forwarding == true) && (orionldState.uriParams.local == false);
  if (distributed == false)
  {
    LM(("KZ3: Need to create the response, well, fix the arrays that define the response later"));
    kjTreeLog(responseErrors, "KZ3: responseErrors");
    kjTreeLog(responseSuccess, "KZ3: responseSuccess");
  }
  else
  {
    ForwardPending* exclusiveList = regMatchForBatchDelete(RegModeAuxiliary, FwdDeleteBatch, entityIdAndTypeTable);
    ForwardPending* redirectList  = regMatchForBatchDelete(RegModeRedirect,  FwdDeleteBatch, entityIdAndTypeTable);
    ForwardPending* inclusiveList = regMatchForBatchDelete(RegModeInclusive, FwdDeleteBatch, entityIdAndTypeTable);
    ForwardPending* distReqList;

    distReqList = forwardingListsMerge(exclusiveList,  redirectList);
    distReqList = forwardingListsMerge(distReqList, inclusiveList);

    distReqLog(distReqList, "Before distReqsMergeForBatchDelete");
    distReqsMergeForBatchDelete(distReqList);
    distReqLog(distReqList, "After distReqsMergeForBatchDelete");

    LM(("KZ2: distReqList at %p", distReqList));
    if (distReqList != NULL)
      LM(("KZ2: distReqList->next%p", distReqList->next));

    if (distReqList != NULL)
    {
      // Enqueue all forwarded requests
      // Now that we've found all matching registrations we can add ourselves to the X-forwarded-For header
      char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);

      int forwards = 0;
      for (ForwardPending* distReqP = distReqList; distReqP != NULL; distReqP = distReqP->next)
      {
        // Send the forwarded request and await all responses
        if (distReqP->regP != NULL)
        {
          char dateHeader[70];
          snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

          if (forwardRequestSend(distReqP, dateHeader, xff) == 0)
          {
            ++forwards;
            distReqP->error = false;
          }
          else
          {
            LM_W(("Forwarded request failed"));
            distReqP->error = true;
          }
        }
      }

      int stillRunning = 1;
      int loops        = 0;

      while (stillRunning != 0)
      {
        CURLMcode cm = curl_multi_perform(orionldState.curlFwdMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlFwdMultiP, NULL, 0, 1000, NULL);
          if (cm != CURLM_OK)
          {
            LM_E(("Internal Error (curl_multi_wait: error %d", cm));
            break;
          }
        }

        if ((++loops >= 50) && ((loops % 25) == 0))
          LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
      }

      if (loops >= 100)
        LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));

      // Wait for responses
      if (forwards > 0)
      {
        CURLMsg* msgP;
        int      msgsLeft;

        while ((msgP = curl_multi_info_read(orionldState.curlFwdMultiP, &msgsLeft)) != NULL)
        {
          if (msgP->msg != CURLMSG_DONE)
            continue;

          if (msgP->data.result == CURLE_OK)
          {
            ForwardPending* drP = fwdPendingLookupByCurlHandle(distReqList, msgP->easy_handle);

            curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &drP->httpResponseCode);
            LM(("Got a response HTTP Status for Reg '%s': %d", drP->regP->regId, drP->httpResponseCode));
            if (drP->rawResponse != NULL)
            {
              LM(("Got a response BODY for Reg '%s': '%s'", drP->regP->regId, drP->rawResponse));

              drP->responseBody = kjParse(orionldState.kjsonP, drP->rawResponse);

              //
              // All Entity IDs in "success" of drP->body to be merged into "success" of finalResponseP, if not already present
              // Also, if any of those in "success" of drP->body are in finalResponseP::error, those need to be removed from finalResponseP::error
              // Also, Entity IDs in "error" of drP->body that are not found anywhere in finalResponseP, those need to be added to finalResponseP::error
              //
              responseMerge(drP, responseSuccess, responseErrors);
            }

            --forwards;
          }
        }
      }
    }
  }


  //
  // Creating the response tree
  // o If no errors - 204
  // o if any error - 207
  //
  if (responseErrors->value.firstChildP == NULL)
    orionldState.httpStatusCode  = 204;
  else
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(response, responseErrors);
    kjChildAdd(response, responseSuccess);

    orionldState.responseTree    = response;
    orionldState.httpStatusCode  = 207;

    if (troe)
    {
      //
      // For TRoE, orionldState.requestTree needs to be an array of the IDs of all the entities that have been deleted.
      // In the case of 207, and especially if Distributed Operations, then the "responseSuccess" is the array with that information.
      // Luckily it has the exact format that troePostBatchDelete needs, so, we just make 'orionldState.requestTree' point to 'responseSuccess'.
      //
      orionldState.requestTree = responseSuccess;
    }

    kjTreeLog(responseSuccess, "responseSuccess");
  }

  orionldState.out.contentType = JSON;
  orionldState.noLinkHeader    = true;

  return true;
}
