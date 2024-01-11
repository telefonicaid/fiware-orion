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
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjClone.h"                                          // kjClone
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // Lmt*

#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/types/QNode.h"                                    // QNode
#include "orionld/types/RegCache.h"                                 // RegCache
#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/common/orionldError.h"                            // orionldError
#include "orionld/legacyDriver/legacyGetEntities.h"                 // legacyGetEntities
#include "orionld/kjTree/kjTreeLog.h"                               // kjTreeLog
#include "orionld/q/qLex.h"                                         // qLex
#include "orionld/q/qParse.h"                                       // qParse
#include "orionld/q/qClone.h"                                       // qClone
#include "orionld/payloadCheck/pCheckGeo.h"                         // pCheckGeo
#include "orionld/distOp/distOpRequests.h"                          // distOpRequests
#include "orionld/distOp/distOpListsMerge.h"                        // distOpListsMerge
#include "orionld/distOp/distOpListDebug.h"                         // distOpListDebug
#include "orionld/distOp/xForwardedForMatch.h"                      // xForwardedForMatchº
#include "orionld/distOp/viaMatch.h"                                // viaMatch
#include "orionld/distOp/distOpCreate.h"                            // distOpCreate
#include "orionld/regMatch/regMatchOperation.h"                     // regMatchOperation
#include "orionld/regMatch/regMatchInformationArrayForQuery.h"      // regMatchInformationArrayForQuery
#include "orionld/serviceRoutines/orionldGetEntitiesDistributed.h"  // orionldGetEntitiesDistributed
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntities.h"             // Own interface



// ----------------------------------------------------------------------------
//
// qCheck -
//
static QNode* qCheck(char* qString)
{
  QNode* qList;
  char*  title;
  char*  detail;

  qList = qLex(qString, true, &title, &detail);
  if (qList == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
    LM_RE(NULL, ("Error (qLex: %s: %s)", title, detail));
  }

  QNode* qNode = qParse(qList, NULL, true, true, &title, &detail);  // 3rd parameter: forDb=true
  if (qNode == NULL)
  {
    orionldError(OrionldBadRequestData, title, detail, 400);
    LM_E(("Error (qParse: %s: %s) - but, the subscription will be inserted in the sub-cache without 'q'", title, detail));
  }

  return qNode;
}



// -----------------------------------------------------------------------------
//
// pCheckQueryParams -
//
bool pCheckQueryParams(char* id, char* type, char* idPattern, char* q, char* geometry, char* attrs, bool local, EntityMap* entityMap, QNode** qNodeP, OrionldGeoInfo* geoInfoP)
{
  //
  // URI param validity check
  //

  if ((id        == NULL)  &&
      (idPattern == NULL)  &&
      (type      == NULL)  &&
      (geometry  == NULL)  &&
      (attrs     == NULL)  &&
      (q         == NULL)  &&
      (local     == false) &&
      (orionldState.in.entityMap == NULL))
  {
    orionldError(OrionldBadRequestData,
                 "Too broad query",
                 "Need at least one of: entity-id, entity-type, geo-location, attribute-list, Q-filter, local=true, or an entity map",
                 400);

    return false;
  }


  //
  // If ONE or ZERO types in URI param 'type', the prepared array isn't used, just a simple char-pointer (named "type")
  //
  if      (orionldState.in.typeList.items == 0) type = (char*) ".*";
  else if (orionldState.in.typeList.items == 1) type = orionldState.in.typeList.array[0];

  if (pCheckGeo(geoInfoP, orionldState.uriParams.geometry, orionldState.uriParams.georel, orionldState.uriParams.coordinates, orionldState.uriParams.geoproperty) == false)
    return false;

  QNode* qNode = NULL;
  if (orionldState.uriParams.q != NULL)
  {
    qNode = qCheck(orionldState.uriParams.q);
    if (qNode == NULL)
      return false;
  }

  *qNodeP = qNode;

  return true;
}



// -----------------------------------------------------------------------------
//
// regMatchForEntitiesQuery -  FIXME: Move to orionld/forwarding/regMatchForEntitiesQuery.cpp/h
//
DistOp* regMatchForEntitiesQuery
(
  RegistrationMode  regMode,
  StringArray*      idListP,
  StringArray*      typeListP,
  StringArray*      attrListP
)
{
  DistOp* distOpList = NULL;

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    if ((regP->mode & regMode) == 0)
      continue;

    if (regMatchOperation(regP, DoQueryEntity) == false)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Operation (operation == QueryEntity)", regP->regId));
      continue;
    }

    // Loop detection
    if (viaMatch(orionldState.in.via, regP->hostAlias) == true)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Loop (Via)", regP->regId));
      continue;
    }

    if (xForwardedForMatch(orionldState.in.xForwardedFor, regP->ipAndPort) == true)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Loop (X-Forwarded-For)", regP->regId));
      continue;
    }

    DistOp* distOpP = regMatchInformationArrayForQuery(regP, idListP, typeListP, attrListP);
    if (distOpP == NULL)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Information Array", regP->regId));
      continue;
    }

    //
    // Add distOpP to the linked list (distOpList)
    //
    LM_T(LmtRegMatch, ("%s: Reg Match !", regP->regId));

    distOpList = distOpListsMerge(distOpList, distOpP);
  }

  return distOpList;
}



// ----------------------------------------------------------------------------
//
// distOpRequestsForEntitiesQuery -
//
DistOp* distOpRequestsForEntitiesQuery(char* idPattern, QNode* qNode)
{
  // FIXME: idPattern, qNode also need to be taken into account inside regMatchForEntitiesQuery
  DistOp* auxiliarList  = regMatchForEntitiesQuery(RegModeAuxiliary, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* exclusiveList = regMatchForEntitiesQuery(RegModeExclusive, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* redirectList  = regMatchForEntitiesQuery(RegModeRedirect,  &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  // FIXME: Strip off attrs, entityId, entityType, etc from URI params (regMatchForEntitiesQuery(RegModeExclusive) already does it for each match

  DistOp* inclusiveList = regMatchForEntitiesQuery(RegModeInclusive, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);
  DistOp* distOpList;

  distOpList = distOpListsMerge(exclusiveList,  redirectList);
  distOpList = distOpListsMerge(distOpList, inclusiveList);
  distOpList = distOpListsMerge(distOpList, auxiliarList);

  return distOpList;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntities -
//
// Steal checks of URI params from legacyGetEntities
//
// Three cases:
// * legacy
// * local query (either by setting "local=true", or by "no matching registrations
// * distributed query (best effort, without freezing time)
//
//
bool orionldGetEntities(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntities();

  char*   id        = orionldState.uriParams.id;         // Validity assured in mhdConnectionTreat.cpp (pCheckEntityIdParam)
  char*   type      = orionldState.uriParams.type;       // Validity assured in mhdConnectionTreat.cpp (pCheckEntityTypeParam)
  char*   idPattern = orionldState.uriParams.idPattern;  // No check
  char*   q         = orionldState.uriParams.q;
  char*   geometry  = orionldState.uriParams.geometry;
  char*   attrs     = orionldState.uriParams.attrs;      // Validity assured in mhdConnectionTreat.cpp (pCheckAttrsParam)
  bool    local     = orionldState.uriParams.local;
  QNode*  qNode     = NULL;

  LM_T(LmtEntityMap, ("onlyIds: %s", (orionldState.uriParams.onlyIds == true)? "TRUE" : "FALSE"));

  // According to the spec, id takes precedence over idPattern, so, if both are present, idPattern is NULLed out
  if ((orionldState.in.idList.items > 0) && (orionldState.uriParams.idPattern != NULL))
    idPattern = NULL;

  OrionldGeoInfo geoInfo;
  if (pCheckQueryParams(id, type, idPattern, q, geometry, attrs, local, orionldState.in.entityMap, &qNode, &geoInfo) == false)
    return false;

  //
  // Distributed GET /entities, using the concept "entityMaps" is disabled by default.
  // To turn it on, please start Orion-LD with the CLI option:
  //   -wip entityMaps
  // Or, the env var:
  // export ORIONLD_WIP=entityMaps
  //
  if (entityMapsEnabled == false)
    orionldState.distributed = false;

  //
  // Entity Maps can be turned on using the HTTP header ORIONLD-WIP
  //
  if (orionldState.in.wip != NULL)
  {
    if (strcmp(orionldState.in.wip, "entityMaps") == 0)
      orionldState.distributed = true;
  }

  if (orionldState.in.entityMap != NULL)
  {
    // No query params can be used when asking for pages in an entity map
    if ((id       != NULL) || (type  != NULL) || (idPattern != NULL) || (q != NULL)      ||
        (geometry != NULL) || (attrs != NULL) || (orionldState.uriParams.local == true))
    {
      orionldError(OrionldBadRequestData, "Query parameters present", "not allowed when paginating using an entity map", 400);
      return false;
    }
  }

  if (orionldState.distributed == false)
    return orionldGetEntitiesLocal(&orionldState.in.typeList,
                                   &orionldState.in.idList,
                                   &orionldState.in.attrList,
                                   idPattern,
                                   qNode,
                                   &geoInfo,
                                   orionldState.uriParams.lang,
                                   orionldState.uriParamOptions.sysAttrs,
                                   orionldState.uriParams.geometryProperty,
                                   orionldState.uriParams.onlyIds,
                                   false);

  if (orionldState.in.entityMap == NULL)  // No prior entity map is requested - must create a new one
  {
    //
    // Find matching registrations
    //
    orionldState.distOpList = distOpRequestsForEntitiesQuery(idPattern, qNode);

    //
    // if there are no matching registrations, the request is treated as a local request
    //
    if (orionldState.distOpList == NULL)
      return orionldGetEntitiesLocal(&orionldState.in.typeList,
                                     &orionldState.in.idList,
                                     &orionldState.in.attrList,
                                     idPattern,
                                     qNode,
                                     &geoInfo,
                                     orionldState.uriParams.lang,
                                     orionldState.uriParamOptions.sysAttrs,
                                     orionldState.uriParams.geometryProperty,
                                     orionldState.uriParams.onlyIds,
                                     true);

    // Create the "@none" DistOp
    DistOp* local  = distOpCreate(DoQueryEntity, NULL, &orionldState.in.idList, &orionldState.in.typeList, &orionldState.in.attrList);

    // Add lang, geometryProperty, qNode, ...   to the "@none" DistOp
    local->lang                = (orionldState.uriParams.lang != NULL)? strdup(orionldState.uriParams.lang) : NULL;
    local->geoInfo.geometry    = geoInfo.geometry;
    local->geoInfo.georel      = geoInfo.georel;
    local->geoInfo.coordinates = (geoInfo.coordinates != NULL)? kjClone(NULL, geoInfo.coordinates) : NULL;
    local->geoInfo.minDistance = geoInfo.minDistance;
    local->geoInfo.maxDistance = geoInfo.maxDistance;
    local->geoInfo.geoProperty = (geoInfo.geoProperty != NULL)? strdup(geoInfo.geoProperty) : NULL;
    local->geometryProperty    = (orionldState.uriParams.geometryProperty != NULL)? strdup(orionldState.uriParams.geometryProperty) : NULL;
    local->qNode               = (qNode != NULL)? qClone(qNode) : NULL;

    // Add the "@none" DistOp to the linked list of DistOps
    local->next    = orionldState.distOpList;
    orionldState.distOpList = local;

    // This is the DistOp list to be used over pagination - it is "malloqued" so it survives the current request
    distOpListDebug2(orionldState.distOpList, "distOpList for Entity Query");
    LM_T(LmtDistOpList, ("--------------- "));
  }

  return orionldGetEntitiesDistributed(orionldState.distOpList, idPattern, qNode, &geoInfo);
}
