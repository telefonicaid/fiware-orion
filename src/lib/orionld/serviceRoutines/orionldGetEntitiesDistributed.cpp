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
#include "kjson/kjRender.h"                                         // kjRender (for debugging purposes - LM_T)
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/common/uuidGenerate.h"                            // uuidGenerate
#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/q/QNode.h"                                        // QNode
#include "orionld/mongoc/mongocEntitiesQuery.h"                     // mongocEntitiesQuery
#include "orionld/dbModel/dbModelToEntityIdAndTypeObject.h"         // dbModelToEntityIdAndTypeObject
#include "orionld/forwarding/DistOp.h"                              // DistOp
#include "orionld/forwarding/distOpsSend.h"                         // distOpsSend
#include "orionld/forwarding/distOpLookupByCurlHandle.h"            // distOpLookupByCurlHandle
#include "orionld/forwarding/distOpListDebug.h"                     // distOpListDebug2
#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntitiesPage.h"         // orionldGetEntitiesPage
#include "orionld/serviceRoutines/orionldGetEntitiesDistributed.h"  // Own interface



// -----------------------------------------------------------------------------
//
// Implementation details
//
// - To be able to do pagination, we need a list of all matching entities:
//
//     "orionldEntityMap": {
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
// 3. Merge all the reponses into one single array (orionldEntityMap)
// 4. Now we have a list and we can do pagination.
// 5. Just pick the start index and end index of orionldEntityMap and:
//    - Identify each registered endpoint to be queried (from start index to end index)
//    - Gather all entity ids for each registered endpoint
//    - query using (GET /entities?id=E1,E2,E3,...En)
//    - Actually, if one endpoint has multiple registrations, the attrs param may differ between registrations
//    - Will probably need one GET /entities per registration, unfortunately :(
// 6. Merge all results into a single entity array, and especially merge entities of the same id
// 7. Respond to the initial GET /entities
//



// -----------------------------------------------------------------------------
//
// DistOpResponseTreatFunction -
//
typedef int (*DistOpResponseTreatFunction)(DistOp* distOpP, void* callbackParam);



// -----------------------------------------------------------------------------
//
// distOpsReceive - FIXME: move to orionld/forwarding/distOpsReceive.cpp/h
//
void distOpsReceive(DistOp* distOpList, DistOpResponseTreatFunction treatFunction, void* callbackParam)
{
  LM_T(LmtSR, ("Receiving responses"));
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



//
// Entity Map
//
// Right now it looks like this:
// {
//   "urn:entities:E1": [ "urn:registrations:R1", "urn:registrations:R2", ... "@none" ],
//   "urn:entities:E1": [ "urn:registrations:R1", "urn:registrations:R2", ... ],
// }
//
// which is later turned into:
// {
//   "urn:registrations:R1": ["urn:entities:E1", "urn:entities:E2" ],
//   "urn:registrations:R2": ["urn:entities:E1", "urn:entities:E2" ],
//   "@none":                ["urn:entities:E1"]
// }
//
// BUT, I need also entity type and attrs ...
//
// New Entity Map:
// {
//   "urn:entities:E1": {
//     "regs": [ "urn:registrations:R1", "urn:registrations:R2", ... "@none" ]
//     "type": "T",
//     "attrs": [ "urn:attribute:P1", "urn:attribute:P2", ... ]
//   }
// }
//
// And turn it into (not only the array of entity id, but also, type, attrs, ... ALL URL params):
// {
//   "urn:registrations:R1": [
//     {
//       "ids": "urn:entities:E1,urn:entities:E2",
//       "type": "T",
//       "attrs": "urn:attribute:P1,urn:attribute:P2,...,urn:attribute:P2",
//       "q": "xxx",
//       "geoQ": {}
//     },
//   ]
// }
//
// BUT, I already store all that info in the DistOp structure ...
// Well, not all perhaps - I'd have to add a few things
// So, what if I give the DistOp items an ID and I add that ID (so I can find it later)
//
// New Entity Map II:
// {
//   "urn:entities:E1": [ "DistOp0001", "DistOp0002", "DistOp0003", ... "@none" ],
// }
//
// And turn that into:
// {
//   "DistOp0001": [ "urn:entities:E1", "urn:entities:E2" ],
//   "DistOp0002": [ "urn:entities:E1", "urn:entities:E2" ],
//   "@none":      [ "urn:entities:E1", "urn:entities:E2" ],
// }
//
// Does local have a DistOp?
// It will need to - the URL params from the first request (when the Entity Map was created) must be saved
//

// -----------------------------------------------------------------------------
//
// entityMapItemAdd -
//
static void entityMapItemAdd(const char* entityId, DistOp* distOpP)
{
  KjNode* matchP = kjLookup(orionldEntityMap, entityId);

  if (matchP == NULL)
  {
    //
    // The entity ID is not present in the list - must be added
    //
    LM_T(LmtEntityMap, ("The entity ID '%s' is not present in the list - adding it", entityId));
    matchP = kjArray(NULL, entityId);
    kjChildAdd(orionldEntityMap, matchP);
  }

  //
  // Add DistOp ID to matchP's array - remember it's a global variable, can't use kaAlloc
  //
  // const char*  distOpId      = (distOpP != NULL)? distOpP->id : "@none";
  const char*  distOpId      = (distOpP != NULL)? distOpP->regP->regId : "@none";
  KjNode*      distOpIdNodeP = kjString(NULL, NULL, distOpId);

  LM_T(LmtEntityMap, ("Adding DistOp '%s' to entity '%s'", distOpId, matchP->name));
  kjChildAdd(matchP, distOpIdNodeP);
}



// -----------------------------------------------------------------------------
//
// idListResponse - callback function for distOpMatchIdsGet
//
int idListResponse(DistOp* distOpP, void* callbackParam)
{
  if ((distOpP->httpResponseCode == 200) && (distOpP->responseBody != NULL))
  {
    LM_T(LmtEntityMapDetail, ("Entity map from registration '%s' (distOp at %p)", distOpP->regP->regId, distOpP));
    kjTreeLog(distOpP->responseBody, "distOpP->responseBody", LmtEntityMap);
    for (KjNode* eIdNodeP = distOpP->responseBody->value.firstChildP; eIdNodeP != NULL; eIdNodeP = eIdNodeP->next)
    {
      char* entityId = eIdNodeP->value.s;

      LM_T(LmtEntityMap, ("o Entity '%s', distOp '%s', registration '%s'", entityId, distOpP->id, distOpP->regP->regId));
      entityMapItemAdd(entityId, distOpP);
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

  distOpListDebug2(distOpList, "DistOps before sending the onlyId=true requests");
  // Send all distributed requests
  int forwards = distOpsSend(distOpList);

  // Await all responses, if any
  if (forwards > 0)
    distOpsReceive(distOpList, idListResponse, orionldEntityMap);
}



// -----------------------------------------------------------------------------
//
// entityMapCreate -
//
static void entityMapCreate(DistOp* distOpList, char* idPattern, QNode* qNode, OrionldGeoInfo* geoInfoP)
{
  orionldEntityMap = kjObject(NULL, "orionldEntityMap");
  LM_T(LmtDistOpList, ("Created an entity map at %p", orionldEntityMap));

  //
  // Send requests to all matching registration-endpoints, to fill in the entity map
  //
  distOpMatchIdsRequest(distOpList);  // Not including local hits

  //
  // if there are no entity hits to the matching registrations, the request is treated as a local request
  //
  if (orionldEntityMap->value.firstChildP == NULL)
    return;

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == GEOJSON)
    geojsonGeometryLongName = orionldState.in.geometryPropertyExpanded;

  // Get the local matches
  KjNode* localEntityV   = NULL;
  LM_T(LmtMongoc, ("orionldState.in.attrList.items: %d", orionldState.in.attrList.items));
  LM_T(LmtMongoc, ("Calling mongocEntitiesQuery"));

  //
  // Can't do any pagination in this step, and we only really need the Entity ID
  // Need to teporarily modify the users input for that
  //
  int offset = orionldState.uriParams.offset;
  int limit  = orionldState.uriParams.limit;

  orionldState.uriParams.offset = 0;
  orionldState.uriParams.limit  = 1000;

  KjNode* localDbMatches = mongocEntitiesQuery(&orionldState.in.typeList,
                                             &orionldState.in.idList,
                                             idPattern,
                                             &orionldState.in.attrList,
                                             qNode,
                                             geoInfoP,
                                             NULL,
                                             geojsonGeometryLongName,
                                             true);

  orionldState.uriParams.offset = offset;
  orionldState.uriParams.limit  = limit;

  kjTreeLog(localDbMatches, "localDbMatches", LmtSR);
  if (localDbMatches != NULL)
  {
    localEntityV = dbModelToEntityIdAndTypeObject(localDbMatches);
    LM_T(LmtEntityMap, ("Adding local entities to the entityMap"));
    kjTreeLog(localEntityV, "localEntityV", LmtEntityMap);
    for (KjNode* eidNodeP = localEntityV->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
    {
      const char* entityId = eidNodeP->value.s;

      LM_T(LmtEntityMap, ("o Entity '%s', distOp 'local'", entityId));
      entityMapItemAdd(entityId, NULL);
    }
  }

  orionldEntityMapCount = kjChildCount(orionldEntityMap);

  // <DEBUG>
  if (lmTraceIsSet(LmtEntityMap) == true)
  {
    int ix = 0;

    LM_T(LmtEntityMap, ("Entity Maps (%d):", orionldEntityMapCount));
    kjTreeLog(orionldEntityMap, "orionldEntityMap", LmtEntityMap);

    for (KjNode* entityP = orionldEntityMap->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      char rBuf[1024];

      bzero(rBuf, 1024);
      kjFastRender(entityP, rBuf);
      LM_T(LmtEntityMap, ("  %03d '%s': %s", ix, entityP->name, rBuf));
      ++ix;
    }
  }
  // </DEBUG>
}



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesDistributed -
//
bool orionldGetEntitiesDistributed(DistOp* distOpList, char* idPattern, QNode* qNode, OrionldGeoInfo* geoInfoP)
{
  LM_T(LmtMongoc, ("orionldState.in.attrList.items: %d", orionldState.in.attrList.items));
  if (orionldEntityMap != NULL)
  {
    LM_T(LmtMongoc, ("Calling orionldGetEntitiesPage"));
    return orionldGetEntitiesPage();
  }

  LM_T(LmtMongoc, ("orionldState.in.attrList.items: %d", orionldState.in.attrList.items));
  entityMapCreate(distOpList, idPattern, qNode, geoInfoP);
  LM_T(LmtMongoc, ("orionldState.in.attrList.items: %d", orionldState.in.attrList.items));

  //
  // if there are no entity hits to the matching registrations, the request is treated as a local request
  //
  if (orionldEntityMap->value.firstChildP == NULL)
    return orionldGetEntitiesLocal(&orionldState.in.typeList,
                                   &orionldState.in.idList,
                                   &orionldState.in.attrList,
                                   idPattern,
                                   qNode,
                                   geoInfoP,
                                   orionldState.uriParams.lang,
                                   orionldState.uriParamOptions.sysAttrs,
                                   orionldState.uriParams.geometryProperty,
                                   orionldState.uriParams.onlyIds,
                                   true);

  // Must return the ID of the entity map as an HTTP header
  uuidGenerate(orionldEntityMapId, sizeof(orionldEntityMapId), "urn:ngsi-ld:entity-map:");
  orionldHeaderAdd(&orionldState.out.headers, HttpEntityMap, orionldEntityMapId, 0);

  return orionldGetEntitiesPage();
}
