/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include <postgresql/libpq-fe.h>                               // PGconn

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/pgSubAttributePush.h"                   // pgSubAttributePush
#include "orionld/troe/pgObservedAtExtract.h"                  // pgObservedAtExtract
#include "orionld/troe/pgSubAttributeTreat.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// pgSubAttributeTreat - treat a sub-attribute for db insertion
//
bool pgSubAttributeTreat
(
  PGconn*      connectionP,
  KjNode*      subAttrP,
  const char*  entityId,
  const char*  attributeId
)
{
  if (subAttrP->type != KjObject)
    LM_RE(false, ("Sub Attribute '%s' is not an Object", subAttrP->name));

  char*   subAttributeType;
  KjNode* typeP = kjLookup(subAttrP, "type");

  if (typeP == NULL)
    LM_RE(false, ("Sub-Attribute '%s' has no type", subAttrP->name));
  subAttributeType = typeP->value.s;

  kjChildRemove(subAttrP, typeP);

  char instanceId[80];
  uuidGenerate(instanceId, sizeof(instanceId), true);

  //
  // Get info from all special sub-sub-attributes.
  // Normal sub-sub-attributes are not used for TRoE - they're thrown away.
  // - No need to remove anything from the tree - just extract info and send to pgSubAttributePush
  //
  char*    observedAt  = NULL;
  KjNode*  valueNodeP  = NULL;
  char*    unitCode    = NULL;

  for (KjNode* subSubAttrP = subAttrP->value.firstChildP; subSubAttrP != NULL; subSubAttrP = subSubAttrP->next)
  {
    if      (strcmp(subSubAttrP->name, "observedAt") == 0)  observedAt = pgObservedAtExtract(subSubAttrP);
    else if (strcmp(subSubAttrP->name, "value")      == 0)  valueNodeP = subSubAttrP;
    else if (strcmp(subSubAttrP->name, "object")     == 0)  valueNodeP = subSubAttrP;
    else if (strcmp(subSubAttrP->name, "unitCode")   == 0)  unitCode   = subSubAttrP->value.s;
  }

  // Push the sub-attribute to DB
  LM_TMP(("TEMP: Push the sub-attribute '%s' to DB", subAttrP->name));
  if (pgSubAttributePush(connectionP, valueNodeP, instanceId, subAttributeType, entityId, attributeId, subAttrP->name, observedAt, unitCode) == false)
  {
    LM_E(("Internal Error (pgAttributePush failed)"));
    return false;
  }

  return true;
}
