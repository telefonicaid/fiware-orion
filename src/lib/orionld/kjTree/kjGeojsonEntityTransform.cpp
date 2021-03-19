/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjArray, kjChildAdd, kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldCoreContext.h"                  // ORIONLD_CORE_CONTEXT_URL
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjGeojsonEntityTransform - transform a KjNode tree into geo+json format
//
// PARAMETERS
//   * tree:               the KjNode tree (in normalized form), that is to be transformed into geo+json form
//   * keyValues:          Is options=keyValues enabled?
//   * geoPropertyNode:    if ?attrs=X is used, anf the GeoProperty is not part of the list, then
//                         the geo-property might come in this parameter, instead of inside the tree itself
//
//
// About GeoJSON Representation of an Entity in the NGSI-LD API spec:
// --------------------------------------------------------------------------------
//
// - Default GeoProperty is "location"
// - The URL parameter "geometryProperty" can be used to use an arbitrary GeoProperty
// - If the GeoProperty has multiple instances (datasetId):
//   - Use the default instance (if the URL parameter 'datasetId' is not set)
//   - Else, use the matching one (same datasetId) - and indicate the datasetId in the response
// - If the GeoProperty doesn't exist (or is of the wrong type), then the geometry shall be undefined
//   and returned with a value of null - which is syntactically valid GeoJSON.
// - The entity type (as 'type' is used to express 'Feature', is added as a special property name "type",
//   and the value of this special property is just the value of the entity type.
//
// OUTPUT FORMAT:
// {
//   "id": "entity id",
//   "type": "Feature",
//   "geometry": { "type": "Point", "coordinates": [1,2] }  # Note, NO subattrs nor timestamps nor ...
//   "properties": [
//     "type": "The type of the entity"
//     "P1": { "type": "Property", "value": ... },
//     "P2": {},
//     "R1": {},
//     "R2": {},
//     ...
//   ]
// }
//
KjNode* kjGeojsonEntityTransform(KjNode* tree, bool keyValues, KjNode* geoPropertyNode)
{
  //
  // 1. Find and remove 'id' and 'type' from the original entity
  //
  KjNode* idP   = kjLookup(tree, "id");
  KjNode* typeP = kjLookup(tree, "type");

  kjChildRemove(tree, idP);
  kjChildRemove(tree, typeP);

  //
  // Create the new tree, add 'id' and 'type' from the old tree
  //
  KjNode* geojsonTreeP = kjObject(orionldState.kjsonP, NULL);
  KjNode* geojsonTypeP = kjString(orionldState.kjsonP, "type", "Feature");
  KjNode* propertiesP  = kjObject(orionldState.kjsonP, "properties");
  KjNode* typeProperty;

  kjChildAdd(geojsonTreeP, idP);
  kjChildAdd(geojsonTreeP, geojsonTypeP);
  kjChildAdd(geojsonTreeP, propertiesP);


  //
  // Should the @context be added to the payload body?
  //
  if (orionldState.linkHeaderAdded == false)
  {
    KjNode* contextP;

    if (orionldState.link == NULL)
      contextP = kjString(orionldState.kjsonP, "@context", ORIONLD_CORE_CONTEXT_URL);
    else
      contextP = kjString(orionldState.kjsonP, "@context", orionldState.link);

    kjChildAdd(geojsonTreeP, contextP);
  }

  //
  // Add the entity type under properties::type as a special property
  //
  typeProperty = kjString(orionldState.kjsonP, "type", typeP->value.s);
  kjChildAdd(propertiesP, typeProperty);

  //
  // Get the geometry property
  // No expansion of 'geoPropertyName' is necessary/needed as the tree context has been compressed already
  // FIXME: Only problem is if orionldState.uriParams.geometryProperty is given in its expanded form ...
  //
  KjNode* geoPropertyP = NULL;

  if (geoPropertyNode == NULL)
  {
    if (orionldState.geoPropertyMissing == false)
    {
      const char* geoPropertyName = (orionldState.uriParams.geometryProperty == NULL)? "location" : orionldState.uriParams.geometryProperty;

      geoPropertyP = kjLookup(tree, geoPropertyName);
    }
  }
  else
    geoPropertyP = geoPropertyNode;

  if (geoPropertyP != NULL)
  {
    if (geoPropertyP->type == KjObject)
    {
      //
      // Might be either only the value or the entire GeoProperty
      //
      KjNode* type = kjLookup(geoPropertyP, "type");

      if ((type != NULL) && (strcmp(type->value.s, "GeoProperty") == 0))
      {
        geoPropertyP = kjLookup(geoPropertyP, "value");

        if (geoPropertyP != NULL)
        {
          geoPropertyP = kjClone(orionldState.kjsonP, geoPropertyP);
          geoPropertyP->name = (char*) "geometry";
        }
        else
          geoPropertyP = kjNull(orionldState.kjsonP, "geometry");
      }
      else
      {
        geoPropertyP = kjClone(orionldState.kjsonP, geoPropertyP);
        geoPropertyP->name = (char*) "geometry";
      }
    }
    else
      geoPropertyP = kjNull(orionldState.kjsonP, "geometry");
  }
  else
    geoPropertyP = kjNull(orionldState.kjsonP, "geometry");

  kjChildAdd(geojsonTreeP, geoPropertyP);

  //
  // The rest of the properties
  //
  KjNode* treeAttrP = tree->value.firstChildP;
  KjNode* next;

  while (treeAttrP != NULL)
  {
    next = treeAttrP->next;
    kjChildRemove(tree, treeAttrP);
    kjChildAdd(propertiesP, treeAttrP);
    treeAttrP = next;
  }

  return geojsonTreeP;
}
