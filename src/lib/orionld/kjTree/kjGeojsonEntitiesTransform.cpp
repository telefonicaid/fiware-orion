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
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjArray, kjChildAdd
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
#include "orionld/kjTree/kjGeojsonEntitiesTransform.h"           // Own interface



// -----------------------------------------------------------------------------
//
// geoPropertyNodeLookup -
//
static KjNode* geoPropertyNodeLookup(KjNode* geoPropertyNodes, KjNode* entityIdNode, const char* geoPropertyName)
{
  if ((entityIdNode == NULL) || (entityIdNode->type != KjString))
    LM_RE(NULL, ("Internal Error (no id field found in entity)"));

  const char* entityId = entityIdNode->value.s;

  // Lookup the entityId in geoPropertyNodes and return
  if (geoPropertyNodes != NULL)
  {
    for (KjNode* entityP = geoPropertyNodes->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      KjNode* idP = kjLookup(entityP, "id");

      if ((idP == NULL) || (strcmp(idP->value.s, entityId) != 0))
        continue;

      // It is OK to return the entire property - kjGeojsonEntityTransform handles both cases
      KjNode* geoP = kjLookup(entityP, geoPropertyName);
      return geoP;
    }
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// kjGeojsonEntitiesTransform -
//
KjNode* kjGeojsonEntitiesTransform
(
  KjNode*      tree,
  const char*  attrs,
  const char*  geometryProperty,
  const char*  preferHeader,
  bool         concise,
  const char*  context
)
{
  KjNode* geojsonTreeP        = kjObject(orionldState.kjsonP, NULL);
  KjNode* geojsonTypeP        = kjString(orionldState.kjsonP, "type", "FeatureCollection");
  KjNode* geojsonFeatureArray = kjArray(orionldState.kjsonP, "features");

  kjChildAdd(geojsonTreeP, geojsonTypeP);
  kjChildAdd(geojsonTreeP, geojsonFeatureArray);

  //
  // If the URI param 'attrs' has been used, and the geometry-property for geo+json is not part of
  // 'attrs', then (an extra query to mongo was done by the service routine to extract the info needed)
  // the geometry-property is taken from orionldState.geoPropertyNodes
  //
  char*       geoProp                 = ((geometryProperty != NULL) && (geometryProperty[0] != 0))? (char*) geometryProperty : (char*) "location";
  bool        geometryPropertyInAttrs = false;  // Lookup in 'attrs'
  char*       linkHeader              = NULL;   // Need this as a parameter to this function
  bool        linkHeaderAdded         = false;  // Need this as a parameter to this function

  // FIXME: This strstr isn't 100% reliable ...
  if (attrs != NULL)
  {
    if (strstr(attrs, geoProp) != NULL)
      geometryPropertyInAttrs = true;
  }

  for (KjNode* entityP = tree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* geoPropertyNode    = NULL;
    bool    geoPropertyMissing = false;

    if ((attrs != NULL) && (geometryPropertyInAttrs == false))
    {
      KjNode* entityIdNode    = kjLookup(entityP, "id");
      geoPropertyNode = geoPropertyNodeLookup(orionldState.geoPropertyNodes, entityIdNode, geoProp);
      if (geoPropertyNode == NULL)
        geoPropertyMissing = true;
    }

    KjNode* geojsonEntityP = kjGeojsonEntityTransform(entityP,
                                                      geoPropertyNode,
                                                      linkHeader,
                                                      preferHeader,
                                                      geoProp,
                                                      geoPropertyMissing,
                                                      linkHeaderAdded,
                                                      concise,
                                                      context);

    kjChildAdd(geojsonFeatureArray, geojsonEntityP);
  }

  return geojsonTreeP;
}
