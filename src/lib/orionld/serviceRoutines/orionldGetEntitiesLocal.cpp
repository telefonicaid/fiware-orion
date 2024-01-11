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
#include <unistd.h>                                                 // NULL
#include <stdint.h>                                                 // types: uint64_t, ...

extern "C"
{
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjBuilder.h"                                        // kjString, kjObject, kjChildAdd, ...
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjClone.h"                                          // kjClone
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/OrionldHeader.h"                            // orionldHeaderAdd, HttpResultsCount
#include "orionld/types/OrionldGeoInfo.h"                           // OrionldGeoInfo
#include "orionld/types/QNode.h"                                    // QNode
#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/context/orionldContextItemExpand.h"               // orionldContextItemExpand
#include "orionld/mongoc/mongocEntitiesQuery.h"                     // mongocEntitiesQuery
#include "orionld/kjTree/kjChildPrepend.h"                          // kjChildPrepend
#include "orionld/dbModel/dbModelToApiEntity.h"                     // dbModelToApiEntity2
#include "orionld/dbModel/dbModelToEntityIdAndTypeObject.h"         // dbModelToEntityIdAndTypeObject
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // Own interface



// -----------------------------------------------------------------------------
//
// apiEntityToGeoJson - transform an API Entity into a GEOJSON Entity
//
KjNode* apiEntityToGeoJson(KjNode* apiEntityP, KjNode* geometryNodeP, bool geoPropertyFromProjection)
{
  KjNode* propertiesP = kjObject(orionldState.kjsonP, "properties");

  //
  // 1. Find 'type' in the original entity - remove it and wait until everything is moved to 'properties' before putting 'type' back
  //
  KjNode* typeP = kjLookup(apiEntityP, "type");
  if (typeP != NULL)  // Can't really be NULL, can it?
    kjChildRemove(apiEntityP, typeP);


  //
  // 2. Find the 'id' in the original entity and remove it temporarily - will be put back again once 'properties' has been filled
  //
  KjNode* idP = kjLookup(apiEntityP, "id");
  if (idP != NULL)  // Can't really be NULL, can it?
    kjChildRemove(apiEntityP, idP);

  // 3. In case the geometryProperty was added to the projection even though it was not part of the "attrs" URI param - just remove it
  if ((geoPropertyFromProjection == true) && (geometryNodeP != NULL))
    kjChildRemove(apiEntityP, geometryNodeP);

  // 4. Move EVERYTHING from "apiEntityP" to "properties"
  propertiesP->value.firstChildP = apiEntityP->value.firstChildP;
  propertiesP->lastChild         = apiEntityP->lastChild;

  // Clear out apiEntityP
  apiEntityP->value.firstChildP  = NULL;
  apiEntityP->lastChild          = NULL;

  //
  // Should the @context be added to the payload body?
  //
  if (orionldState.linkHeaderAdded == false)
  {
    // Only if Prefer is not set to body=json
    if ((orionldState.preferHeader == NULL) || (strcasecmp(orionldState.preferHeader, "body=json") != 0))
    {
      KjNode* contextP;

      if (orionldState.link == NULL)
        contextP = kjString(orionldState.kjsonP, "@context", coreContextUrl);
      else
        contextP = kjString(orionldState.kjsonP, "@context", orionldState.link);

      kjChildAdd(apiEntityP, contextP);
      orionldState.noLinkHeader = true;
    }
  }

  // 5. Put the original entity type inside 'properties'
  if (typeP != NULL)
    kjChildPrepend(propertiesP, typeP);

  // 6. Put the original entity id inside the top level entity
  if (idP != NULL)
    kjChildAdd(apiEntityP, idP);

  // 7. Create the new 'type' for the GEOJSON Entity and add it to the toplevel
  typeP = kjString(orionldState.kjsonP, "type", "Feature");
  kjChildPrepend(apiEntityP, typeP);

  // 8. Create the "geometry" (key-values) top-level item
  KjNode* geometryP = NULL;
  if ((geometryNodeP != NULL) && (geometryNodeP->type == KjObject))
  {
    geometryP = kjLookup(geometryNodeP, "value");
    if (geometryP == NULL)
    {
      // "value" not found ... can it be Simplified format?
      geometryP = geometryNodeP;
    }

    if (geometryP != NULL)
    {
      if (geometryP->type == KjObject)  // && hasChildren type+coordinates ...
      {
        geometryP = kjClone(orionldState.kjsonP, geometryP);
        geometryP->name = (char*) "geometry";
      }
      else
        geometryP = NULL;
    }
  }

  if (geometryP == NULL)
    geometryP = kjNull(orionldState.kjsonP, "geometry");

  kjChildAdd(apiEntityP, geometryP);

  // 9. Adding all the properties to top-level
  propertiesP->next = NULL;
  kjChildAdd(apiEntityP, propertiesP);

  return apiEntityP;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesLocal -
//
bool orionldGetEntitiesLocal
(
  StringArray*     typeList,
  StringArray*     idList,
  StringArray*     attrList,
  char*            idPattern,
  QNode*           qNode,
  OrionldGeoInfo*  geoInfoP,
  const char*      lang,
  bool             sysAttrs,
  const char*      geometryProperty,
  bool             onlyIds,
  bool             countHeaderAlreadyAdded
)
{
  char* geojsonGeometryLongName = NULL;

  if (orionldState.out.contentType == MT_GEOJSON)
  {
    if ((geometryProperty != NULL) && (strcmp(geometryProperty, "location") != 0))
      geojsonGeometryLongName = orionldContextItemExpand(orionldState.contextP, geometryProperty, true, NULL);
    else
      geojsonGeometryLongName = (char*) "location";
  }

  int64_t       count = 0;
  KjNode*       dbEntityArray = mongocEntitiesQuery(typeList,
                                                    idList,
                                                    idPattern,
                                                    attrList,
                                                    qNode,
                                                    geoInfoP,
                                                    &count,
                                                    geojsonGeometryLongName,
                                                    onlyIds);

  if (dbEntityArray == NULL)
    return false;

  if (onlyIds == true)
  {
    orionldState.responseTree = dbModelToEntityIdAndTypeObject(dbEntityArray);
    orionldState.noLinkHeader = true;
  }
  else
  {
    KjNode*              apiEntityArray  = kjArray(orionldState.kjsonP, NULL);
    OrionldRenderFormat  rf              = RF_NORMALIZED;

    if      (orionldState.uriParamOptions.concise   == true) rf = RF_CONCISE;
    else if (orionldState.uriParamOptions.keyValues == true) rf = RF_SIMPLIFIED;

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

        KjNode*     apiEntityP           = dbModelToApiEntity2(dbEntityP, sysAttrs, rf, lang, true, &orionldState.pd);
        const char* geometryPropertyName = (geometryProperty == NULL)? "location" : geometryProperty;
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
  }

  if ((orionldState.uriParams.count == true) && (countHeaderAlreadyAdded == false))
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  // If empty result array, no Link header is needed
  if (orionldState.responseTree->value.firstChildP == NULL)
    orionldState.noLinkHeader = true;

  return true;
}
