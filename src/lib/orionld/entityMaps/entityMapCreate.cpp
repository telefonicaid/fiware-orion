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
#include <stdlib.h>                                                 // malloc

extern "C"
{
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjParse.h"                                          // kjParse
#include "kjson/kjBuilder.h"                                        // kjObject
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjRender.h"                                         // kjFastRender (for debugging purposes - LM_T)
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/EntityMap.h"                                // EntityMap
#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/types/QNode.h"                                    // QNode
#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/types/DistOpListItem.h"                           // DistOpListItem
#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/common/uuidGenerate.h"                            // uuidGenerate
#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/kjTree/kjSort.h"                                  // kjStringArraySort
#include "orionld/distOp/distOpLookupByCurlHandle.h"                // distOpLookupByCurlHandle
#include "orionld/distOp/distOpListDebug.h"                         // distOpListDebug2
#include "orionld/distOp/distOpsSend.h"                             // distOpsSend
#include "orionld/mongoc/mongocEntitiesQuery.h"                     // mongocEntitiesQuery
#include "orionld/dbModel/dbModelToEntityIdAndTypeObject.h"         // dbModelToEntityIdAndTypeObject
#include "orionld/entityMaps/entityMapItemAdd.h"                    // entityMapItemAdd
#include "orionld/entityMaps/entityMapCreate.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// DistOpResponseTreatFunction -
//
typedef int (*DistOpResponseTreatFunction)(DistOp* distOpP, void* callbackParam);



// -----------------------------------------------------------------------------
//
// distOpsReceive - FIXME: move to orionld/distOp/distOpsReceive.cpp/h
//
void distOpsReceive(DistOp* distOpList, DistOpResponseTreatFunction treatFunction, void* callbackParam, int requestsSent)
{
  LM_T(LmtCount, ("Receiving %d responses", requestsSent));
  //
  // Read the responses to the forwarded requests
  //
  CURLMsg* msgP;
  int      msgsLeft;
  int      responses = 0;

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

      LM_T(LmtDistOpResponse, ("%s: received a %d response for a forwarded request; %s", distOpP->regP->regId, distOpP->httpResponseCode, distOpP->rawResponse));

      if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
        distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);

      treatFunction(distOpP, callbackParam);
      ++responses;
    }
  }

  LM_W(("********************** Expected %d responses, got %d", requestsSent, responses));
}



// -----------------------------------------------------------------------------
//
// idListResponse - callback function for distOpMatchIdsGet
//
static int idListResponse(DistOp* distOpP, void* callbackParam)
{
  EntityMap* entityMap = (EntityMap*) callbackParam;

  if ((distOpP->httpResponseCode == 200) && (distOpP->responseBody != NULL))
  {
    kjTreeLog(distOpP->responseBody, "DistOp RESPONSE", LmtCount);
    for (KjNode* eIdNodeP = distOpP->responseBody->value.firstChildP; eIdNodeP != NULL; eIdNodeP = eIdNodeP->next)
    {
      // FIXME: The response is supposed to be an array of entity ids
      //        However, when 3 inter-registered brokers run, I get the second response instead of
      //        the first - the first response seems to go missing somewhere...
      //        This is an UGLY attempt to "make it work"
      //
      KjNode* eP = (eIdNodeP->type == KjObject)? kjLookup(eIdNodeP, "id") : eIdNodeP;
      LM_T(LmtCount, ("JSON Type of array item: %s", kjValueType(eIdNodeP->type)));

      char* entityId = eP->value.s;

      LM_T(LmtEntityMap, ("o Entity '%s', distOp '%s', registration '%s'", entityId, distOpP->id, distOpP->regP->regId));
      entityMapItemAdd(entityMap, entityId, distOpP);
    }
  }

  return 0;
}



// -----------------------------------------------------------------------------
//
// distOpMatchIdsRequest -
//
static void distOpMatchIdsRequest(DistOp* distOpList, EntityMap* entityMap)
{
  if (distOpList == NULL)
    return;

  distOpListDebug2(distOpList, "DistOps before sending the onlyId=true requests");
  // Send all distributed requests
  int forwards = distOpsSend(distOpList, orionldState.in.aerOS);

  // Await all responses, if any
  if (forwards > 0)
    distOpsReceive(distOpList, idListResponse, entityMap, forwards);
}



// -----------------------------------------------------------------------------
//
// entityMapCreate
//
EntityMap* entityMapCreate(DistOp* distOpList, char* idPattern, QNode* qNode, OrionldGeoInfo* geoInfoP)
{
  EntityMap* entityMap = (EntityMap*) malloc(sizeof(EntityMap));
  if (entityMap == NULL)
    LM_X(1, ("Out of memory allocating a memory map"));

  entityMap->map = kjObject(NULL, "EntityMap");
  if (entityMap->map == NULL)
    LM_X(1, ("Out of memory allocating a memory map"));

  uuidGenerate(entityMap->id, sizeof(entityMap->id), "urn:ngsi-ld:entity-map:");

  LM_T(LmtDistOpList, ("Created an entity map at %p (%s)", entityMap, entityMap->id));

  //
  // Send requests to all matching registration-endpoints, to fill in the entity map
  //
  distOpMatchIdsRequest(distOpList, entityMap);  // Not including local hits

  kjTreeLog(entityMap->map, "entityMap", LmtSR);

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == MT_GEOJSON)
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
                                               true,
                                               false);

  orionldState.uriParams.offset = offset;
  orionldState.uriParams.limit  = limit;

  if (localDbMatches != NULL)
  {
    localEntityV = dbModelToEntityIdAndTypeObject(localDbMatches);
    LM_T(LmtEntityMap, ("Adding local entities to the entityMap"));

    for (KjNode* eidNodeP = localEntityV->value.firstChildP; eidNodeP != NULL; eidNodeP = eidNodeP->next)
    {
      const char* entityId = eidNodeP->value.s;

      LM_T(LmtEntityMap, ("o Entity '%s', distOp 'local'", entityId));
      entityMapItemAdd(entityMap, entityId, NULL);
    }
  }

  // Sort all regs of the entities
  for (KjNode* entityP = entityMap->map->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    kjStringArraySort(entityP);
  }

  entityMap->count = kjChildCount(entityMap->map);

#if 0
  // ------------------- <DEBUG>
  if (lmTraceIsSet(LmtEntityMap) == true)
  {
    int ix = 0;

    LM_T(LmtEntityMap, ("Entity Maps (%d):", entityMap->count));
    kjTreeLog(entityMap->map, "EntityMap", LmtEntityMap);

    for (KjNode* entityP = entityMap->map->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      char rBuf[1024];

      bzero(rBuf, 1024);
      kjFastRender(entityP, rBuf);
      LM_T(LmtEntityMap, ("  %03d '%s': %s", ix, entityP->name, rBuf));
      ++ix;
    }
  }
  kjTreeLog(entityMap->map, "EntityMap", LmtSR);
  // ---------------- </DEBUG>
#endif

  return entityMap;
}
