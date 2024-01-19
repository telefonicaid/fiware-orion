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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, ...
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/TreeNode.h"                              // TreeNode
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/legacyDriver/legacyPostQuery.h"                // legacyPostQuery
#include "orionld/payloadCheck/pCheckQuery.h"                    // pCheckQuery
#include "orionld/mongoc/mongocEntitiesQuery2.h"                 // mongocEntitiesQuery2
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/serviceRoutines/orionldPostQuery.h"            // Own interface



// -----------------------------------------------------------------------------
//
// apiEntityToGeoJson - FIXME: Own module in orionld/common
//
extern KjNode* apiEntityToGeoJson(KjNode* apiEntityP, KjNode* geometryNodeP, bool geoPropertyFromProjection);



// ----------------------------------------------------------------------------
//
// orionldPostQuery -
//
bool orionldPostQuery(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostQuery();

  TreeNode* treeNodeV = pCheckQuery(orionldState.requestTree);
  if (treeNodeV == NULL)
    return false;

  //
  // pCheckQuery has left us everything checked and expanded.
  // Now we need to transform stuff so that we can call mongocEntitiesQuery
  // Actually, we can't use mongocEntitiesQuery, unfortunately, because:
  //   POST Query    has EntitySelector
  //   GET Entities  has typeList+idList
  // and they are not compatible :(((
  //

  //
  // Getting the relevant values from the output from pCheckQuery
  // Four parameters are passed via orionldState:
  // - orionldState.uriParams.offset  (URL parameter)
  // - orionldState.uriParams.limit   (URL parameter)
  // - orionldState.uriParams.count   (URL parameter)
  // - orionldState.tenantP           (HTTP header)
  //
  int64_t          count;
  KjNode*          entitySelectorP = treeNodeV[1].nodeP;
  KjNode*          attrsArray      = treeNodeV[2].nodeP;
  QNode*           qTree           = (QNode*) treeNodeV[3].output;
  OrionldGeoInfo*  geoInfoP        = (OrionldGeoInfo*) treeNodeV[4].output;
  char*            lang            = (treeNodeV[9].nodeP != NULL)? treeNodeV[9].nodeP->value.s : NULL;
  KjNode*          dbEntityArray   = mongocEntitiesQuery2(entitySelectorP, attrsArray, qTree, geoInfoP, lang, &count);

  //
  // The post-processing (after the call to mongoc query entities) I simply copied from orionldGetEntities()
  //
  if (dbEntityArray == NULL)
    return false;

  KjNode*              apiEntityArray  = kjArray(orionldState.kjsonP, NULL);
  OrionldRenderFormat  rf              = orionldState.out.format;

  if (orionldState.out.contentType == MT_GEOJSON)
  {
    KjNode* geojsonToplevelP = kjObject(orionldState.kjsonP, NULL);
    KjNode* featuresP        = kjArray(orionldState.kjsonP, "features");  // this is where all entities go
    KjNode* typeP            = kjString(orionldState.kjsonP, "type", "FeatureCollection");

    kjChildAdd(geojsonToplevelP, typeP);
    kjChildAdd(geojsonToplevelP, featuresP);

    orionldState.responseTree = geojsonToplevelP;

    KjNode* dbEntityP = dbEntityArray->value.firstChildP;
    KjNode* next;

    while (dbEntityP != NULL)
    {
      next = dbEntityP->next;

      // Must remove dbEntityP from dbEntityArray as dbEntityP gets transformed into apiEntityP and then inserted into featuresP
      kjChildRemove(dbEntityArray, dbEntityP);

      KjNode*     apiEntityP           = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, rf, lang, true, &orionldState.pd);
      const char* geometryPropertyName = (orionldState.uriParams.geometryProperty == NULL)? "location" : orionldState.uriParams.geometryProperty;
      KjNode*     geometryNodeP        = kjLookup(apiEntityP, geometryPropertyName);

      apiEntityP = apiEntityToGeoJson(apiEntityP, geometryNodeP, orionldState.geoPropertyFromProjection);
      kjChildAdd(featuresP, apiEntityP);

      dbEntityP = next;
    }
  }
  else
  {
    orionldState.responseTree = apiEntityArray;

    for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      KjNode* apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, rf, lang, true, &orionldState.pd);
      kjChildAdd(apiEntityArray, apiEntityP);
    }
  }

  if (orionldState.uriParams.count == true)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  // If empty result array, no Link header is needed
  if (orionldState.responseTree->value.firstChildP == NULL)
    orionldState.noLinkHeader = true;

  return true;
}
