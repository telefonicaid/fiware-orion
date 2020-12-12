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
#include "orionld/temporal/pgSubAttributePush.h"               // pgSubAttributePush
#include "orionld/temporal/pgSubAttributeTreat.h"              // Own interface



// -----------------------------------------------------------------------------
//
// pgSubAttributeTreat - treat a sub-attribute for db insertion
//
bool pgSubAttributeTreat
(
  PGconn*      connectionP,
  KjNode*      subAttrP,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeRef,
  const char*  attributeId,
  const char*  createdAt,
  const char*  modifiedAt
)
{
  LM_TMP(("TEMP: treating sub-attr '%s'", subAttrP->name));

  //
  // This can't happen - Orion-LD would have flagged an error and we wouldn't have gotten this far ...
  // Cause, sub-attrs cannot have datasetIds.
  // However, THIS CHECK avoids possible crashes due to bugs and it's beyond fast, so ...
  //
  if (subAttrP->type != KjObject)
    LM_RE(false, ("Sub Attribute '%s' is not an Object", subAttrP->name));


  KjNode* typeP = kjLookup(subAttrP, "type");
  char*   subAttributeType;

  if (typeP == NULL)
    LM_RE(false, ("Sub-Attribute '%s' has no type", subAttrP->name));
  subAttributeType = typeP->value.s;

  kjChildRemove(subAttrP, typeP);

  char     instanceId[64];
  char*    id         = subAttrP->name;
  char*    unitCode   = NULL;
  char*    observedAt = NULL;
  KjNode*  valueNodeP = NULL;

  uuidGenerate(instanceId);

  //
  // Get info from all special sub-sub-attributes.
  // Normal sub-sub-attributes are not used for temporal.
  //
  for (KjNode* subSubAttrP = subAttrP->value.firstChildP; subSubAttrP != NULL; subSubAttrP = subSubAttrP->next)
  {
    if      (strcmp(subSubAttrP->name, "observedAt") == 0)  observedAt = subSubAttrP->value.s;
    else if (strcmp(subSubAttrP->name, "value")      == 0)  valueNodeP = subSubAttrP;
    else if (strcmp(subSubAttrP->name, "object")     == 0)  valueNodeP = subSubAttrP;
    else if (strcmp(subSubAttrP->name, "unitCode")   == 0)  unitCode   = subSubAttrP->value.s;
  }

  // Push the sub-attribute to DB
  LM_TMP(("TEMP: Push the sub-attribute '%s' to DB", id));
  if (pgSubAttributePush(connectionP, valueNodeP, instanceId, subAttributeType, entityRef, entityId, attributeRef, attributeId, id, observedAt, createdAt, modifiedAt, unitCode) == false)
  {
    LM_E(("Internal Error (pgAttributePush failed)"));
    return false;
  }

  return true;
}
