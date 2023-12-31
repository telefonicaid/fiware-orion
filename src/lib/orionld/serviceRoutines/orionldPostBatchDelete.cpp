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

#include "orionld/types/DistOp.h"                              // DistOp
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/entityLookupById.h"                   // entityLookupBy_id_Id
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/legacyDriver/legacyPostBatchDelete.h"        // legacyPostBatchDelete
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/mongoc/mongocEntitiesExist.h"                // mongocEntitiesExist
#include "orionld/mongoc/mongocEntitiesDelete.h"               // mongocEntitiesDelete
#include "orionld/distOp/distOpListsMerge.h"                   // distOpListsMerge
#include "orionld/distOp/distOpSend.h"                         // distOpSend
#include "orionld/distOp/distOpLookupByCurlHandle.h"           // distOpLookupByCurlHandle
#include "orionld/distOp/distOpListDebug.h"                    // distOpListDebug2
#include "orionld/distOp/distOpListRelease.h"                  // distOpListRelease
#include "orionld/distOp/xForwardedForCompose.h"               // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                         // viaCompose
#include "orionld/regMatch/regMatchForBatchDelete.h"           // regMatchForBatchDelete
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
void distReqLog(DistOp* distOpList, const char* title)
{
  LM_T(LmtSR, ("DR: %s", title));
  for (DistOp* drP = distOpList; drP != NULL; drP = drP->next)
  {
    LM_T(LmtSR, ("DR: Registration:               %s", drP->regP->regId));
    LM_T(LmtSR, ("DR: Verb:                       %s", "TBD"));
    LM_T(LmtSR, ("DR: URL:                        %s", "TBD"));  // IP+port should be in the URL
    LM_T(LmtSR, ("DR: Entity ID:                  %s", drP->entityId));
    LM_T(LmtSR, ("DR: Entity Type:                %s", drP->entityType));

    if ((drP->requestBody != NULL) && (lmTraceIsSet(LmtSR)))
    {
      int   bodySize = kjFastRenderSize(drP->requestBody);
      char* body     = kaAlloc(&orionldState.kalloc, bodySize + 256);

      kjFastRender(drP->requestBody, body);  // Depends on the trace level LmtSR
      LM_T(LmtSR, ("DR: payload body:               %s", body));
    }
    else
      LM_T(LmtSR, ("DR: payload body:               NULL"));

    LM_T(LmtSR, ("DR: -----------------------------------------------------------"));
  }
}



// -----------------------------------------------------------------------------
//
// firstItemOfRegLookup -
//
DistOp* firstItemOfRegLookup(DistOp* distOpList, RegCacheItem* regP)
{
  for (DistOp* drP = distOpList; drP != NULL; drP = drP->next)
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
void distReqsMergeForBatchDelete(DistOp* distOpList)
{
  //
  // Payload body needed for the entity id(s)
  //
  DistOp* drP  = distOpList;
  DistOp* next;
  DistOp* prev = NULL;

  while (drP != NULL)
  {
    next = drP->next;

    //
    // Either it's the first, in which case the drP->requestBody needs to be created (as an array and entityId added to it)
    // Or, the first must be found, and the entityId added to its array (and the drP item to be removed)
    //
    DistOp* firstP = firstItemOfRegLookup(distOpList, drP->regP);

    if (drP == firstP)
    {
      drP->requestBody = kjArray(orionldState.kjsonP, NULL);

      prev = drP;  // Only setting 'prev' here as in the "esle case", drP is skipped
    }
    else
    {
      // Remove drP from list (just skip over it)
      prev->next = next;
    }

    // Now add the entityId to the array of "firstP"
    KjNode* eidNodeP = kjString(orionldState.kjsonP, NULL, drP->entityId);
    kjChildAdd(firstP->requestBody, eidNodeP);

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
static void responseMerge(DistOp* drP, KjNode* responseSuccess, KjNode* responseErrors)
{
  if (drP->httpResponseCode == 204)
  {
    for (KjNode* eidNodeP = drP->requestBody->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
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
  KjNode* dbEntityIdArray = mongocEntitiesExist(orionldState.requestTree, true);

  // Simplify the DB _id array to an KV object   "entityId": "entityType "
  KjNode* entityIdAndTypeTable = NULL;
  bool   dbOperationOk         = false;

  if ((dbEntityIdArray == NULL) || (dbEntityIdArray->value.firstChildP == NULL))
  {
    entityIdAndTypeTable = kjObject(orionldState.kjsonP,  NULL);
  }
  else
  {
    entityIdAndTypeTable = dbModelToEntityIdAndTypeTable(dbEntityIdArray);

    responseSuccess = entityIdAndTypeTableToIdArray(entityIdAndTypeTable);
    dbOperationOk = mongocEntitiesDelete(responseSuccess);
    if (dbOperationOk == false)
    {
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
  }

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


  // Get the list of Forwarded requests, for matching registrations
  if (orionldState.distributed == true)
  {
    DistOp* exclusiveList = regMatchForBatchDelete(RegModeAuxiliary, DoDeleteBatch, entityIdAndTypeTable);
    DistOp* redirectList  = regMatchForBatchDelete(RegModeRedirect,  DoDeleteBatch, entityIdAndTypeTable);
    DistOp* inclusiveList = regMatchForBatchDelete(RegModeInclusive, DoDeleteBatch, entityIdAndTypeTable);
    DistOp* distOpList;

    distOpList = distOpListsMerge(exclusiveList,  redirectList);
    distOpList = distOpListsMerge(distOpList, inclusiveList);

    distReqLog(distOpList, "Before distReqsMergeForBatchDelete");
    distReqsMergeForBatchDelete(distOpList);
    distReqLog(distOpList, "After distReqsMergeForBatchDelete");

    if (distOpList != NULL)
    {
      // Enqueue all forwarded requests
      // Now that we've found all matching registrations we can add ourselves to the X-forwarded-For header
      char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
      char* via = viaCompose(orionldState.in.via, brokerId);

      int   forwards = 0;
      for (DistOp* distReqP = distOpList; distReqP != NULL; distReqP = distReqP->next)
      {
        // Send the forwarded request and await all responses
        if (distReqP->regP != NULL)
        {
          char dateHeader[70];
          snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

          if (distOpSend(distReqP, dateHeader, xff, via, false, NULL) == 0)
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
        CURLMcode cm = curl_multi_perform(orionldState.curlDoMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
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

        while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
        {
          if (msgP->msg != CURLMSG_DONE)
            continue;

          if (msgP->data.result == CURLE_OK)
          {
            DistOp* drP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);

            curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &drP->httpResponseCode);
            if (drP->rawResponse != NULL)
            {
              drP->responseBody = kjParse(orionldState.kjsonP, drP->rawResponse);

              //
              // All Entity IDs in "success" of drP->requestBody to be merged into "success" of finalResponseP, if not already present
              // Also, if any of those in "success" of drP->requestBody are in finalResponseP::error, those need to be removed from finalResponseP::error
              // Also, Entity IDs in "error" of drP->requestBody that are not found anywhere in finalResponseP, those need to be added to finalResponseP::error
              //
              responseMerge(drP, responseSuccess, responseErrors);
            }

            --forwards;
          }
        }

        distOpListRelease(distOpList);
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
  }

  orionldState.out.contentType = MT_JSON;
  orionldState.noLinkHeader    = true;

  return true;
}
