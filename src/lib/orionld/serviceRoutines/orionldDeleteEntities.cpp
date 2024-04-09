/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/payloadCheck/pCheckQueryParams.h"              // pCheckQueryParams
#include "orionld/mongoc/mongocEntitiesQuery.h"                  // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntitiesDelete.h"                 // mongocEntitiesDelete
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/regMatch/regMatchOperation.h"                  // regMatchOperation
#include "orionld/regMatch/regMatchForEntitiesQuery.h"           // regMatchForEntitiesQuery
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/distOpsSend.h"                          // distOpsSend
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/serviceRoutines/orionldDeleteEntities.h"       // Own Interface



// -----------------------------------------------------------------------------
//
// alterationAdd -
//
static void alterationAdd(char* entityId, char* entityTypeExpanded, char* entityTypeCompacted)
{
  //
  // Create the alteration for notifications
  //
  OrionldAlteration* altP = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  bzero(altP, sizeof(OrionldAlteration));
  altP->entityId   = entityId;
  altP->entityType = entityTypeExpanded;

  //
  // Create a payload body to represent the deletion (to be sent as notification later)
  //
  KjNode* apiEntityP      = kjObject(orionldState.kjsonP, NULL);  // Used for notifications, if needed
  KjNode* idNodeP         = kjString(orionldState.kjsonP, "id", entityId);
  KjNode* deletedAtNodeP  = kjString(orionldState.kjsonP, "deletedAt", orionldState.requestTimeString);

  kjChildAdd(apiEntityP, idNodeP);
  if (entityTypeCompacted != NULL)
  {
    KjNode* typeNodeP    = kjString(orionldState.kjsonP, "type", entityTypeCompacted);
    kjChildAdd(apiEntityP, typeNodeP);
  }
  kjChildAdd(apiEntityP, deletedAtNodeP);

  altP->finalApiEntityP = apiEntityP;

  //
  // Add the alteration to the list
  //
  altP->next               = orionldState.alterations;
  orionldState.alterations = altP;
}



// ----------------------------------------------------------------------------
//
// distOpRequestsForEntitiesPurge -
//
DistOp* distOpRequestsForEntitiesPurge(char* idPattern, QNode* qNode)
{
  // FIXME: idPattern, qNode also need to be taken into account inside regMatchForEntitiesQuery
  DistOp* auxiliarList  = regMatchForEntitiesQuery(RegModeAuxiliary, DoPurgeEntity, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* exclusiveList = regMatchForEntitiesQuery(RegModeExclusive, DoPurgeEntity, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* redirectList  = regMatchForEntitiesQuery(RegModeRedirect,  DoPurgeEntity, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  // FIXME: Strip off attrs, entityId, entityType, etc from URI params (regMatchForEntitiesQuery(RegModeExclusive) already does it for each match

  DistOp* inclusiveList = regMatchForEntitiesQuery(RegModeInclusive, DoPurgeEntity, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* distOpList;

  distOpList = distOpListsMerge(exclusiveList,  redirectList);
  distOpList = distOpListsMerge(distOpList, inclusiveList);
  distOpList = distOpListsMerge(distOpList, auxiliarList);

  LM_W(("distOpList: %p", distOpList));
  return distOpList;
}



// ----------------------------------------------------------------------------
//
// orionldDeleteEntities -
//
// 01. Retrieve list of entity id+type from local database
// 02. Create the array of alterations - for later comparison with subscriptions
// 03. Create the array of DistOps
// 04. Execute the array of DistOps
// 05. Add to the response body
// 06. Respond
//
bool orionldDeleteEntities(void)
{
  OrionldGeoInfo  geoInfo;
  char*           id           = orionldState.uriParams.id;         // Validity assured in mhdConnectionTreat.cpp (pCheckEntityIdParam)
  char*           type         = orionldState.uriParams.type;       // Validity assured in mhdConnectionTreat.cpp (pCheckEntityTypeParam)
  char*           idPattern    = orionldState.uriParams.idPattern;  // No check
  char*           q            = orionldState.uriParams.q;
  char*           geometry     = orionldState.uriParams.geometry;
  char*           attrs        = orionldState.uriParams.attrs;      // Validity assured in mhdConnectionTreat.cpp (pCheckAttrsParam)
  bool            local        = orionldState.uriParams.local;
  QNode*          qNode        = NULL;
  KjNode*         responseBody = kjObject(orionldState.kjsonP, "response");

  if (pCheckQueryParams(id, type, idPattern, q, geometry, attrs, local, NULL, &qNode, &geoInfo) == false)
    return false;

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == MT_GEOJSON)
  {
    if ((orionldState.uriParams.geometryProperty != NULL) && (strcmp(orionldState.uriParams.geometryProperty, "location") != 0))
      geojsonGeometryLongName = orionldContextItemExpand(orionldState.contextP, orionldState.uriParams.geometryProperty, true, NULL);
    else
      geojsonGeometryLongName = (char*) "location";
  }


  //
  // GET list of entity ids of local matching entities
  //
  int64_t       count = 0;
  KjNode*       localEntityIdArray = kjArray(orionldState.kjsonP, NULL);    // Array of only entity ids - for mongocEntitiesDelete
  KjNode*       dbEntityArray      = mongocEntitiesQuery(&orionldState.in.typeList,
                                                         &orionldState.in.idList,
                                                         idPattern,
                                                         &orionldState.in.attrList,
                                                         qNode,
                                                         &geoInfo,
                                                         &count,
                                                         geojsonGeometryLongName,
                                                         false,
                                                         true);
  //
  // Create the alterations
  //
  if (dbEntityArray != NULL)
  {
    for (KjNode* eP = dbEntityArray->value.firstChildP; eP != NULL; eP = eP->next)
    {
      KjNode* _idP = kjLookup(eP, "_id");
      if (_idP == NULL)
      {
        LM_E(("Entity in Database without '_id'!"));
        continue;
      }

      KjNode* typeP = kjLookup(_idP, "type");
      if (typeP == NULL)
      {
        LM_E(("Entity in Database without 'type'!"));
        continue;
      }

      KjNode* idP = kjLookup(_idP, "id");
      if (idP == NULL)
      {
        LM_E(("Entity in Database without 'id'!"));
        continue;
      }

      char* entityId            = idP->value.s;
      char* entityTypeExpanded  = typeP->value.s;
      char* entityTypeCompacted = orionldContextItemAliasLookup(orionldState.contextP, entityTypeExpanded, NULL, NULL);

      alterationAdd(entityId, entityTypeExpanded, entityTypeCompacted);

      KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, entityId);
      kjChildAdd(localEntityIdArray, idNodeP);  // Saving the entityId in localEntityIdArray - for mongocEntitiesDelete
    }
  }

  //
  // Distributed Ops
  //
  DistOp* distOpList = distOpRequestsForEntitiesPurge(idPattern, qNode);

  if (distOpList != NULL)
  {
    distOpsSend(distOpList, orionldState.in.aerOS);
    distOpResponses(distOpList, responseBody);
    kjTreeLog(responseBody, "responseBody", LmtSR);
    distOpListRelease(distOpList);
  }


  //
  // Delete the entities in the local DB
  //
  int failures = 0;
  if (localEntityIdArray->value.firstChildP != NULL)
  {
    if (mongocEntitiesDelete(localEntityIdArray) == false)
    {
      LM_E(("mongocEntitiesDelete reports an error to delete the entities"));
      for (KjNode* eP = localEntityIdArray; eP != NULL; eP = eP->next)
      {
        distOpFailure(responseBody, NULL, "Database Error", eP->name, 500, NULL);
        ++failures;
      }
    }
    else
    {
      for (KjNode* eP = localEntityIdArray; eP != NULL; eP = eP->next)
      {
        distOpSuccess(responseBody, NULL, eP->value.s, NULL);
      }
    }
  }

  if (failures == 0)
  {
    orionldState.responseTree   = NULL;
    orionldState.httpStatusCode = 204;
  }
  else
  {
    orionldState.responseTree   = responseBody;
    orionldState.httpStatusCode = 207;
  }

  return true;
}
