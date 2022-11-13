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

#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // Own interface



// -----------------------------------------------------------------------------
//
// geoPropertyAsSimplified -
//
// If it really is a GeoProperty (normalized, keyValues or concise) a cloned Simplified representation, with name "geometry", is returned.
// Is it NOT a valid GeoProperty, then a NULL clone is returned (with name "geometry")
//
static KjNode* geoPropertyAsSimplified(KjNode* attributeP)
{
  kjTreeLog(attributeP, "attributeP");
  if (attributeP->type != KjObject)
  {
    LM(("Not an object"));
    return NULL;
  }

  KjNode* typeP = kjLookup(attributeP, "type");
  if (typeP == NULL)
  {
    LM(("No type - might be OK if concise - BUT, in such case there MUST be a value and that value MUST have type + coordinates"));

    if (orionldState.uriParamOptions.concise == true)
    {
      // In 'Concise' attribute format, there may be no type field
      KjNode* valueP = kjLookup(attributeP, "value");

      if (valueP != NULL)
      {
        attributeP = valueP;
        typeP      = kjLookup(attributeP, "type");

        if (typeP == NULL)
        {
          LM(("No type inside 'value' for Concise"));
          return NULL;
        }
      }
    }
    else
      return NULL;
  }

  if (strcmp(typeP->value.s, "GeoProperty") == 0)  // It's a Normalized GeoProperty, return its value
  {
    KjNode* valueP = kjLookup(attributeP, "value");

    if (valueP == NULL)
    {
      LM(("No value"));
      return NULL;
    }

    attributeP = valueP;  // to be cloned and named 'geometry
    typeP      = kjLookup(attributeP, "type");

    if (typeP == NULL)
    {
      LM(("No type inside 'value' for Normalized"));
      return NULL;
    }
  }

  //
  // attributeP now points to the "value" field
  // typeP points to the "type" field inside attributeP
  //
  if ((strcmp(typeP->value.s, "Point")           != 0)  &&
      (strcmp(typeP->value.s, "LineString")      != 0)  &&
      (strcmp(typeP->value.s, "Polygon")         != 0)  &&
      (strcmp(typeP->value.s, "MultiPoint")      != 0)  &&
      (strcmp(typeP->value.s, "MultiLineString") != 0)  &&
      (strcmp(typeP->value.s, "MultiPolygon")    != 0))
  {
    LM(("Not a GeoJSON type (%s)", typeP->value.s));
    return NULL;  // Not a GeoProperty
  }

  KjNode* coordsP = kjLookup(attributeP, "coordinates");

  if ((coordsP == NULL) || (coordsP->type != KjArray))
  {
    LM(("No coords"));
    return NULL;
  }

  LM(("Cloning and returning"));
  KjNode* cloneP = kjClone(orionldState.kjsonP, attributeP);
  cloneP->name = (char*) "geometry";
  return cloneP;
}



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
KjNode* kjGeojsonEntityTransform(KjNode* tree, KjNode* geoPropertyNode)
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
    // Only if Prefer is not set to body=json
    if ((orionldState.preferHeader == NULL) || (strcasecmp(orionldState.preferHeader, "body=json") != 0))
    {
      KjNode* contextP;

      if (orionldState.link == NULL)
        contextP = kjString(orionldState.kjsonP, "@context", coreContextUrl);
      else
        contextP = kjString(orionldState.kjsonP, "@context", orionldState.link);

      kjChildAdd(geojsonTreeP, contextP);
      orionldState.noLinkHeader = true;
    }
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
    geoPropertyP = geoPropertyAsSimplified(geoPropertyP);  // as KeyValues if GeoProperty (and cloned), else NULL
  if (geoPropertyP == NULL)
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
