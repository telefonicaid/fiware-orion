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
#include "orionld/temporal/pgAttributePush.h"                  // pgAttributePush
#include "orionld/temporal/pgSubAttributeTreat.h"              // pgSubAttributeTreat
#include "orionld/temporal/pgAttributeTreat.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// pgAttributeTreat - treat an attribute for db insertion
//
bool pgAttributeTreat
(
  PGconn*      connectionP,
  KjNode*      attrP,
  const char*  entityRef,
  const char*  entityId,
  const char*  createdAt,
  const char*  modifiedAt
)
{
  if (attrP->type == KjArray)
    LM_RE(false, ("Attribute is an array ... datasetId? Sorry - not yet implemented"));

  KjNode* typeP = kjLookup(attrP, "type");
  char*   attributeType;

  if (typeP == NULL)
    LM_RE(false, ("Attribute '%s' has no type", attrP->name));
  attributeType = typeP->value.s;

  kjChildRemove(attrP, typeP);

  char     instanceId[64];
  char*    id         = attrP->name;
  char*    unitCode   = NULL;
  char*    datasetId  = NULL;
  char*    observedAt = NULL;
  KjNode*  valueNodeP = NULL;
  bool     subAttrs   = false;

  uuidGenerate(instanceId);

  //
  // Get all special sub-attributes and remove them from the tree
  // Only normal sub attributes left in the tree after this loop
  // The sub-attributes must be added to the DB after the attribute, as the InstanceID of the attribute is
  // needed (and referenced => the DB gices error if the referenced attribute doesn't exist already).
  //
  // I wonder if this "REFERENCED BY" slows the DB down when inserting ... If so, do we really need it?
  //
  KjNode* subAttrP = attrP->value.firstChildP;
  KjNode* next;

  while (subAttrP != NULL)
  {
    next = subAttrP->next;

    if (strcmp(subAttrP->name, "observedAt") == 0)
      observedAt = subAttrP->value.s;
    else if (strcmp(subAttrP->name, "datasetId") == 0)
      datasetId = subAttrP->value.s;
    else if (strcmp(subAttrP->name, "value") == 0)
      valueNodeP = subAttrP;
    else if (strcmp(subAttrP->name, "object") == 0)
      valueNodeP = subAttrP;
    else if (strcmp(subAttrP->name, "unitCode") == 0)
      unitCode = subAttrP->value.s;
    else
    {
      subAttrs = true;
      subAttrP = next;
      continue;   // pgSubAttributeTreat()  later - after creating the attribute
    }

    kjChildRemove(attrP, subAttrP);
    subAttrP = next;
  }

  // Push the attribute to DB
  if (pgAttributePush(connectionP, valueNodeP, attributeType, entityRef, entityId, id, instanceId, datasetId, observedAt, createdAt, modifiedAt, subAttrs, unitCode) == false)
  {
    LM_E(("Internal Error (pgAttributePush failed)"));
    return false;
  }

  for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
  {
    LM_TMP(("TEMP: Got a sub-attr: '%s'", subAttrP->name));
    if (pgSubAttributeTreat(connectionP, subAttrP, entityRef, entityId, instanceId, id, createdAt, modifiedAt) == false)
    {
      LM_E(("Internal Error (pgSubAttributeTreat failed)"));
      return false;
    }
  }

  return true;
}
