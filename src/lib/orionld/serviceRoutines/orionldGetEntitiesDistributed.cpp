/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjFree.h"                                           // kjFree
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjBuilder.h"                                        // kjChildRemove, kjChildAdd, kjArray
#include "kjson/kjParse.h"                                          // kjParse
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/q/QNode.h"                                        // QNode
#include "orionld/mongoc/mongocEntitiesQuery.h"                     // mongocEntitiesQuery
#include "orionld/forwarding/DistOp.h"                              // DistOp
#include "orionld/forwarding/distOpListDebug.h"                     // distOpListDebug
#include "orionld/forwarding/distOpLookupByCurlHandle.h"            // distOpLookupByCurlHandle
#include "orionld/forwarding/xForwardedForCompose.h"                // xForwardedForCompose
#include "orionld/forwarding/distOpSend.h"                          // distOpSend
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntitiesDistributed.h"  // Own interface



// -----------------------------------------------------------------------------
//
// Implementation details
//
// - To be able to do pagination, we need a list of all matching entities:
//
//     "orionldDistOpMatchIds": {
//       "urn:E1": [ null ],          # null indicates it was found locally
//       "urn:E2": [ null ],
//       "urn:E3": [ null ],
//       "urn:E4": [ null ]
//     }
//
// - As entities can be distributed, the ID of each distributed entity needs to be an array of matching registrations:
//
//     "urn:E1": [ null, "urn:R1", "urn:R3", ... ]
//
// So, we need to ask every matching broker (URI params id,idPattern,type,attrs, ...    VS registrations) for their list of
// entity ids.
// This is not part of the NGSI-LD API, right now. I'll try to push this for inclusion in the API.
// What is needed is for "GET /entities" to have a URL param '?entityIdArray' and just return an array of entity ids - all of them.
// Until this gets included in the API, this mechanism will only work in a federation of 100% Orion-LD brokers
//
// Mechanism:
// 1. GET a list of all matching registrations.
// 2. Query them all (locally as well) for their 'Entity Id Array'
// 3. Merge all the reponses into one single array (orionldDistOpMatchIds)
// 4. Now we have a list and we can do pagination.
// 5. Just pick the start index and end index of orionldDistOpMatchIds and:
//    - Identify each registered endpoint to be queried (from start index to end index)
//    - Gather all entity ids for each registered endpoint
//    - query using (GET /entities?id=E1,E2,E3,...En)
//    - Actually, if one endpoint has multiple registrations, the attrs param may differ between registrations
//    - Will probably need one GET /entities per registration, unfortunately :(
// 6. Merge all results into a single entity array, and especially merge entities of the same id
// 7. Respond to the initial GET /entities
//



// Global variable - Needs to go to orionld/common/orionldState.h/cpp
KjNode* orionldDistOpMatchIds = NULL;



// -----------------------------------------------------------------------------
//
// distOpsSend -
//
int distOpsSend(DistOp* distOpList)
{
  char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);

  char dateHeader[70];
  snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);  // MOVE to orionldStateInit, for example

  int forwards = 0;  // Debugging purposees
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    // Send the forwarded request and await all responses
    if ((distOpP->regP != NULL) && (distOpP->error == false))
    {
      distOpP->onlyIds = true;

      if (distOpSend(distOpP, dateHeader, xff) == 0)
        distOpP->error = false;
      else
        distOpP->error = true;

      ++forwards;  // Also when error?
    }
  }

  int stillRunning = 1;
  int loops        = 0;

  if (forwards > 0)
  {
    while (stillRunning != 0)
    {
      CURLMcode cm = curl_multi_perform(orionldState.curlDoMultiP, &stillRunning);
      if (cm != 0)
      {
        LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
        return -1;
      }

      if (stillRunning != 0)
      {
        cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
        if (cm != CURLM_OK)
        {
          LM_E(("Internal Error (curl_multi_wait: error %d", cm));
          return -2;
        }
      }

      if ((++loops >= 50) && ((loops % 25) == 0))
        LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
    }

    if (loops >= 100)
      LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));
  }

  return forwards;
}



// -----------------------------------------------------------------------------
//
// DistOpResponseTreatFunction -
//
typedef int (*DistOpResponseTreatFunction)(DistOp* distOpP, void* callbackParam);



// -----------------------------------------------------------------------------
//
// distOpsReceive -
//
void distOpsReceive(DistOp* distOpList, DistOpResponseTreatFunction treatFunction, void* callbackParam)
{
  //
  // Read the responses to the forwarded requests
  //
  CURLMsg* msgP;
  int      msgsLeft;

  while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
  {
    if (msgP->msg != CURLMSG_DONE)
      continue;

    if (msgP->data.result == CURLE_OK)
    {
      DistOp* distOpP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);

      if (distOpP == NULL)
      {
        LM_E(("Unable to find the curl handle of a message, presumably a response to a forwarded request"));
        continue;
      }

      curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &distOpP->httpResponseCode);

      if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
        distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);

      LM_T(LmtDistOpResponse, ("%s: received a response for a forwarded request", distOpP->regP->regId, distOpP->httpResponseCode));
      LM_T(LmtDistOpResponse, ("%s: response for a forwarded request: %s", distOpP->regP->regId, distOpP->rawResponse));

      treatFunction(distOpP, callbackParam);
    }
  }
}



// -----------------------------------------------------------------------------
//
// orionldDistOpMatchAdd -
//
void orionldDistOpMatchAdd(const char* entityId, const char* regId)
{
  KjNode* matchP = kjLookup(orionldDistOpMatchIds, entityId);
  if (matchP == NULL)
  {
    //
    // The entity ID is not present in the list - must be added
    //
    matchP = kjArray(NULL, entityId);
    kjChildAdd(orionldDistOpMatchIds, matchP);
  }

  //
  // Add regId to matchP's array - remember it's a global variable, can't use orionldState
  //
  KjNode* regIdNodeP = (regId != NULL)? kjString(NULL, NULL, regId) : kjNull(NULL, NULL);
  kjChildAdd(matchP, regIdNodeP);
}



// -----------------------------------------------------------------------------
//
// idListResponse - callback function for distOpMatchIdsGet
//
int idListResponse(DistOp* distOpP, void* callbackParam)
{
  if ((distOpP->httpResponseCode == 200) && (distOpP->responseBody != NULL))
  {
    for (KjNode* eIdNodeP = distOpP->responseBody->value.firstChildP; eIdNodeP != NULL; eIdNodeP = eIdNodeP->next)
    {
      char* entityId = eIdNodeP->value.s;

      LM_T(LmtSR, ("* Entity '%s', registration '%s'", entityId, distOpP->regP->regId));

      orionldDistOpMatchAdd(entityId, distOpP->regP->regId);
    }
  }

  return 0;
}



// -----------------------------------------------------------------------------
//
// distOpMatchIdsRequest -
//
static void distOpMatchIdsRequest(DistOp* distOpList)
{
  if (distOpList == NULL)
    return;

  // Send all distributed requests
  int forwards = distOpsSend(distOpList);

  // Await all responses, if any
  if (forwards > 0)
    distOpsReceive(distOpList, idListResponse, orionldDistOpMatchIds);
}



// -----------------------------------------------------------------------------
//
// orionldDistOpMatchIdsRelease -
//
void orionldDistOpMatchIdsRelease(void)
{
  if (orionldDistOpMatchIds != NULL)
  {
    kjFree(orionldDistOpMatchIds);
    orionldDistOpMatchIds = NULL;
  }
}



// ----------------------------------------------------------------------------
//
// dbModelToEntityIdArray -
//
// INPUT:  [ { "_id": { "id": "urn:E1" } }, { "_id": { "id": "urn:E2" } }, ... ]
// OUTPUT: [ "urn:E1", "urn:E2", ... ]
//
KjNode* dbModelToEntityIdArray(KjNode* localDbMatches)
{
  KjNode* matchIds = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* dbEntityP = localDbMatches->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    KjNode* _idP = kjLookup(dbEntityP, "_id");

    if (_idP == NULL)
      continue;   // DB Error !!!

    KjNode* idP = kjLookup(_idP, "id");

    if (idP == NULL)
      continue;   // DB Error !!!

    kjChildRemove(_idP, idP);
    kjChildAdd(matchIds, idP);
  }

  return matchIds;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesDistributed -
//
bool orionldGetEntitiesDistributed(DistOp* distOpList, char* idPattern, QNode* qNode, OrionldGeoInfo* geoInfoP)
{
  if ((orionldDistOpMatchIds == NULL) || (orionldState.uriParams.reset == true))
  {
    orionldDistOpMatchIdsRelease();
    orionldDistOpMatchIds = kjObject(NULL, "orionldDistOpMatchIds");
    distOpMatchIdsRequest(distOpList);  // Not including local hits
  }

  //
  // if there are no entity hits to the matching registrations, the request is treated as a local request
  //
  if (orionldDistOpMatchIds == NULL)
    return orionldGetEntitiesLocal(idPattern, qNode, geoInfoP);

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == GEOJSON)
    geojsonGeometryLongName = orionldState.in.geometryPropertyExpanded;

  // Get the local matches
  int64_t count;
  KjNode* localEntityV   = NULL;
  KjNode* localDbMatches = mongocEntitiesQuery(&orionldState.in.typeList,
                                             &orionldState.in.idList,
                                             idPattern,
                                             &orionldState.in.attrList,
                                             qNode,
                                             geoInfoP,
                                             &count,
                                             geojsonGeometryLongName,
                                             true);

  if (localDbMatches != NULL)
  {
    localEntityV = dbModelToEntityIdArray(localDbMatches);

    for (KjNode* eidNodeP = localEntityV->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
    {
      const char* entityId = eidNodeP->value.s;
      orionldDistOpMatchAdd(entityId, NULL);
    }

    kjTreeLog(orionldDistOpMatchIds, "Complete Entity-Reg Array", LmtSR);
  }

  // Now we have the list of entities and their registrations
  // All of that will go into a separatre routine.
  // What will be done here now is:
  //
  // - pick which entities to retrieve (according to pagination)
  //   - Create a new list for that
  //
  // - Send all forwarded requests
  //   - create an idList (?id=E1,E2,...) and add it to each DistOp to be used
  //   - reuse the attrList from the distOp's
  //
  // - Await the responses and
  //   - Merge all responses into an array of entities
  //   - Merge entities into one in case there's more than one with the same Entity ID
  //   - respond to the initial caller
  //
  //
  // ALSO:   Forgot about Count.
  //         Every forwarded request needs to be with "count == true", and the Count HTTP Headers in the responses
  //         must be found and an accumulated count maintained
  //

  orionldState.responseTree   = kjArray(orionldState.kjsonP, NULL);
  orionldState.httpStatusCode = 200;

  LM(("Returning 200"));
  return true;
}
