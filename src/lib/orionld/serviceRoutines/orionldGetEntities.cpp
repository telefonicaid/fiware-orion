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
#include <strings.h>                                             // bzero

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/urlDecode.h"                            // urlDecode
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/types/OrionldGeometry.h"                       // OrionldGeometry
#include "orionld/types/OrionldGeoInfo.h"                        // OrionldGeoInfo
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/legacyDriver/legacyGetEntities.h"              // legacyGetEntities
#include "orionld/mongoc/mongocEntitiesQuery.h"                  // mongocEntitiesQuery
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/q/qLex.h"                                      // qLex
#include "orionld/q/qParse.h"                                    // qParse
#include "orionld/kjTree/kjChildPrepend.h"                       // kjChildPrepend
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/payloadCheck/pCheckGeometry.h"                 // pCheckGeometry
#include "orionld/payloadCheck/pCheckGeorelString.h"             // pCheckGeorelString
#include "orionld/payloadCheck/pCheckGeoCoordinates.h"           // pCheckGeoCoordinates
#include "orionld/serviceRoutines/orionldGetEntities.h"          // Own interface



// -----------------------------------------------------------------------------
//
// geoCheck -
//
static bool geoCheck(OrionldGeoInfo* geoInfoP)
{
  bzero(geoInfoP, sizeof(OrionldGeoInfo));

  if ((orionldState.uriParams.geometry    != NULL) ||
      (orionldState.uriParams.georel      != NULL) ||
      (orionldState.uriParams.coordinates != NULL) ||
      (orionldState.uriParams.geoproperty != NULL))
  {
    //
    // geometry
    //
    if (orionldState.uriParams.geometry == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: geometry", 400);
      return false;
    }
    if (pCheckGeometry(orionldState.uriParams.geometry, &geoInfoP->geometry, false) == false)
      return false;

    //
    // georel
    //
    if (orionldState.uriParams.georel == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: georel", 400);
      return false;
    }

    if (pCheckGeorelString(orionldState.uriParams.georel, geoInfoP) == false)
      return false;

    //
    // coordinates
    //
    if (orionldState.uriParams.coordinates == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "missing: coordinates", 400);
      return false;
    }

    urlDecode(orionldState.uriParams.coordinates);
    geoInfoP->coordinates = kjParse(orionldState.kjsonP, orionldState.uriParams.coordinates);

    if (geoInfoP->coordinates == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Geo-Filter", "invalid coordinates", 400);
      return false;
    }

    if (pCheckGeoCoordinates(geoInfoP->coordinates, geoInfoP->geometry) == false)
      return false;


    //
    // geoproperty
    //
    if ((orionldState.uriParams.geoproperty != NULL) && (strcmp(orionldState.uriParams.geoproperty, "location") != 0))
      geoInfoP->geoProperty = orionldAttributeExpand(orionldState.contextP, orionldState.uriParams.geoproperty, true, NULL);
    else
      geoInfoP->geoProperty = (char*) "location";
  }

  return true;
}



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
    orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
    LM_E(("Error (qParse: %s: %s) - but, the subscription will be inserted in the sub-cache without 'q'", title, detail));
  }

  return qNode;
}



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
// orionldGetEntities -
//
// Steal checks of URI params from legacyGetEntities
//
// NOTE:
//   What if an entity id list is given, but with one single entity id?
//   In general these two requests should be equivalent:
//     - GET /entities?id=urn:ngsi-ld:T:E1
//     - GET /entities/urn:ngsi-ld:T:E1
//
//   However, if more URI params are given, the similarities aren't that big anymore.
//   E.g., if attrs is given:
//     - GET /entities?id=urn:ngsi-ld:T:E1&attrs=P1  [1]
//     - GET /entities/urn:ngsi-ld:T:E1&attrs=P1     [2]
//
//   Imagine that the entity urn:ngsi-ld:T:E1 doesn't have an attribute P1.
//     - The query (1) would give back an empty arry - no match
//     - The retrieval (2) would give back the entity withoput any attributes
//
//   So, perhaps better to play it safe way and NOT EVER "redirect" to GET /entities/{entityId}
//
bool orionldGetEntities(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntities();

  //
  // URI param validity check
  //
  char*                 id             = orionldState.uriParams.id;
  char*                 type           = orionldState.uriParams.type;
  char*                 idPattern      = orionldState.uriParams.idPattern;
  char*                 q              = orionldState.uriParams.q;
  char*                 attrs          = orionldState.uriParams.attrs;
  char*                 geometry       = orionldState.uriParams.geometry;
  bool                  local          = orionldState.uriParams.local;

  if ((id == NULL) && (idPattern == NULL) && (type == NULL) && ((geometry == NULL) || (*geometry == 0)) && (attrs == NULL) && (q == NULL) && (local == false))
  {
    orionldError(OrionldBadRequestData,
                 "Too broad query",
                 "Need at least one of: entity-id, entity-type, geo-location, attribute-list, Q-filter, local=true",
                 400);

    return false;
  }


  //
  // If ONE or ZERO types in URI param 'type', the prepared array isn't used, just a simple char-pointer (named "type")
  //
  if      (orionldState.in.typeList.items == 0) type = (char*) ".*";
  else if (orionldState.in.typeList.items == 1) type = orionldState.in.typeList.array[0];

  OrionldGeoInfo geoInfo;
  if (geoCheck(&geoInfo) == false)
    return false;

  QNode* qNode = NULL;
  if (orionldState.uriParams.q != NULL)
  {
    qNode = qCheck(orionldState.uriParams.q);
    if (qNode == NULL)
      return false;
  }

  // According to the spec, id takes precedence over idPattern, so, if both are present, idPattern is NULLed out
  if ((orionldState.in.idList.items > 0) && (idPattern != NULL))
    idPattern = NULL;

  char* geojsonGeometryLongName = NULL;
  if (orionldState.out.contentType == GEOJSON)
    geojsonGeometryLongName = orionldState.in.geometryPropertyExpanded;

  int64_t       count;
  KjNode*       dbEntityArray = mongocEntitiesQuery(&orionldState.in.typeList,
                                                    &orionldState.in.idList,
                                                    idPattern,
                                                    &orionldState.in.attrList,
                                                    qNode,
                                                    &geoInfo,
                                                    &count,
                                                    geojsonGeometryLongName);

  if (dbEntityArray == NULL)
    return false;

  KjNode*       apiEntityArray  = kjArray(orionldState.kjsonP, NULL);
  RenderFormat  rf              = RF_NORMALIZED;

  if      (orionldState.uriParamOptions.concise   == true) rf = RF_CONCISE;
  else if (orionldState.uriParamOptions.keyValues == true) rf = RF_KEYVALUES;

  if (orionldState.out.contentType == GEOJSON)
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

      KjNode*     apiEntityP           = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, rf, orionldState.uriParams.lang, true, &orionldState.pd);
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
      KjNode* apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, rf, orionldState.uriParams.lang, true, &orionldState.pd);
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
