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
// kjGeojsonEntitiesTransform -
//
KjNode* kjGeojsonEntitiesTransform(KjNode* tree, bool keyValues)
{
  KjNode* geojsonTreeP        = kjObject(orionldState.kjsonP, NULL);
  KjNode* geojsonTypeP        = kjString(orionldState.kjsonP, "type", "FeatureCollection");
  KjNode* geojsonFeatureArray = kjArray(orionldState.kjsonP, "features");

  kjChildAdd(geojsonTreeP, geojsonTypeP);
  kjChildAdd(geojsonTreeP, geojsonFeatureArray);

  for (KjNode* entityP = tree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* geojsonEntityP = kjGeojsonEntityTransform(entityP, keyValues);

    kjChildAdd(geojsonFeatureArray, geojsonEntityP);
  }

  return geojsonTreeP;
}
