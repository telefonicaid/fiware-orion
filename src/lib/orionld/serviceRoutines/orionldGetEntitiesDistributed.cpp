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

#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/types/QNode.h"                                    // QNode
#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/distOp/distOpListRelease.h"                       // distOpListRelease
#include "orionld/entityMaps/entityMapCreate.h"                     // entityMapCreate
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntitiesPage.h"         // orionldGetEntitiesPage
#include "orionld/serviceRoutines/orionldGetEntitiesDistributed.h"  // Own interface



// -----------------------------------------------------------------------------
//
// Implementation details
//
// - To be able to do pagination, we need a list of all matching entities:
//
//     "EntityMap": {
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
// 3. Merge all the reponses into one single array (orionldState,in.EntityMap)
// 4. Now we have a list and we can do pagination.
// 5. Just pick the start index and end index of the EntityMap and:
//    - Identify each registered endpoint to be queried (from start index to end index)
//    - Gather all entity ids for each registered endpoint
//    - query using (GET /entities?id=E1,E2,E3,...En)
//    - Actually, if one endpoint has multiple registrations, the attrs param may differ between registrations
//    - Will probably need one GET /entities per registration, unfortunately :(
// 6. Merge all results into a single entity array, and especially merge entities of the same id
// 7. Respond to the initial GET /entities
//



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


// ----------------------------------------------------------------------------
//
// orionldGetEntitiesDistributed -
//
bool orionldGetEntitiesDistributed(DistOp* distOpList, char* idPattern, QNode* qNode, OrionldGeoInfo* geoInfoP)
{
  if (orionldState.in.entityMap != NULL)
    return orionldGetEntitiesPage();

  LM_T(LmtCount, ("--------------------------- Creating entity map"));
  orionldState.in.entityMap = entityMapCreate(distOpList, idPattern, qNode, geoInfoP);
  distOpListRelease(distOpList);

  LM_T(LmtCount, ("--------------------------- entity map at %p", orionldState.in.entityMap));
  if (orionldState.in.entityMap != NULL)
  {
    LM_T(LmtCount, ("--------------------------- Created entity map"));
    kjTreeLog(orionldState.in.entityMap->map, "EntityMap excl local entities", LmtCount);

    // Add the new entity map to the global list of entity maps
    // sem-take
    orionldState.in.entityMap->next = entityMaps;
    entityMaps = orionldState.in.entityMap;
    // sem-give

    // Must return the ID of the entity map as an HTTP header
    orionldHeaderAdd(&orionldState.out.headers, HttpEntityMap, orionldState.in.entityMap->id, 0);

    if (orionldState.uriParams.limit == 0)
    {
      orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, orionldState.in.entityMap->count);
      orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);
      return true;
    }
  }
  else
    LM_E(("entityMapCreate returned NULL"));

  //
  // if there are no entity hits to the matching registrations, the request is treated as a local request
  //
  if ((orionldState.in.entityMap == NULL) || (orionldState.in.entityMap->map->value.firstChildP == NULL))
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

  return orionldGetEntitiesPage();
}
