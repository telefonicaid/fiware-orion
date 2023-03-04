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
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/q/QNode.h"                                        // QNode
#include "orionld/mongoc/mongocEntitiesQuery.h"                     // mongocEntitiesQuery
#include "orionld/forwarding/DistOp.h"                              // DistOp
#include "orionld/forwarding/distOpListDebug.h"                     // distOpListDebug
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



// -----------------------------------------------------------------------------
//
// distOpMatchIdsGet -
//
static KjNode* distOpMatchIdsGet(DistOp* distOpList)
{
  KjNode* distOpMatchIds = NULL;

  distOpListDebug(distOpList, "Before distOpMatchIdsGet");
  //
  //  Foreach matching reg in distOpList
  //    Query endpoint for an array of matching Entity IDs
  //    Foreach eidMatch
  //      KjNode* matchP = kjLookup(distOpMatchIds, eidMatch->value.s)
  //      if (matchP)
  //        kjChildAdd(matchP, distOpP->regP->regId)
  //      else
  //        matchP = kjArray(NULL, eidMatch->value.s);
  //        kjChildInsert(distOpMatchIds, matchP);
  //
  //

  return distOpMatchIds;
}



// Global variable - Needs to go to orionld/common/orionldState.h/cpp
KjNode* orionldDistOpMatchIds = NULL;



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
    orionldDistOpMatchIds = distOpMatchIdsGet(distOpList);  // Not including local hits
  }

  //
  // if there are no entity hits to the matching registrations, the request is treated as a local request
  //
// #if 0
  // Temporarily removing this, for tests of mongocEntitiesQuery
  if (orionldDistOpMatchIds == NULL)
    return orionldGetEntitiesLocal(idPattern, qNode, geoInfoP);
// #endif

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == GEOJSON)
    geojsonGeometryLongName = orionldState.in.geometryPropertyExpanded;

  // Get the local matches
  KjNode* localEntitiesV = NULL;
  int64_t count;
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
    kjTreeLog(localDbMatches, "Response from mongocEntitiesQuery", LmtMongoc);
    localEntitiesV = dbModelToEntityIdArray(localDbMatches);
    kjTreeLog(localEntitiesV, "Local Entities", LmtMongoc);
  }
  else
    LM(("Found no local matches"));

  //   Foreach DistOp in distOpList
  //     query endpoint for matching entities (only array of entity ID in response)
  //     Foreach id in matchingEidArray
  //       if ((match = kjStringValueLookupInArray(queryIdArray, id)) == true)
  //         kjChildRemove(queryIdArray, match)
  //         idRegArray = kjObject(NULL, match->name)
  //         kjChildAdd(idRegArray, "local");
  //         kjChildAdd(idRegArray, distOpP->regP->regId);
  //       else if ((match = kjLookup()) != NULL)
  // }

  return false;
}
