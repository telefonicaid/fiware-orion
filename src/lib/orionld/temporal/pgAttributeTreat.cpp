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
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/temporal/pgRelationshipPush.h"               // pgRelationshipPush
#include "orionld/temporal/pgStringPropertyPush.h"             // pgStringPropertyPush
#include "orionld/temporal/pgEntityPush.h"                     // Own interface



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
  char*   type;

  if (typeP == NULL)
    LM_RE(false, ("Attribute '%s' has no type", attrP->name));
  type = typeP->value.s;

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
  // Gather special sub-attrs and call pgSubAttributeTreat for normal sub-attrs
  //
  for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
  {
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
      // pgSubAttributeTreat()
    }
  }


  if (strcmp(type, "Relationship") == 0)
  {
    if (pgRelationshipPush(connectionP, valueNodeP->value.s, entityRef, entityId, id, instanceId, datasetId, observedAt, createdAt, modifiedAt, subAttrs) == false)
      LM_RE(false, ("pgRelationshipPush failed"));
  }
  else if (valueNodeP->type == KjString)
  {
    if (pgStringPropertyPush(connectionP, "String", valueNodeP->value.s, entityRef, entityId, id, instanceId, datasetId, observedAt, createdAt, modifiedAt, subAttrs, unitCode) == false)
      LM_RE(false, ("pgStringPropertyPush failed"));
  }
#if 0
  else if (strcmp(type, "GeoProperty") == 0)
    pgGeoPropertyValueTreat(connectionP, valueNodeP, entityRef, entityId, id, instanceId, createdAt, modifiedAt, unitCode);
  else if ((valueNodeP->type == KjArray) || (valueNodeP->type == KjObject))
    pgCompoundPropertyValueTreat(connectionP, valueNodeP, entityRef, entityId, id, instanceId, createdAt, modifiedAt, unitCode);
  else if (valueNodeP->type == KjFloat)
    pgFloatPropertyValueTreat(connectionP, valueNodeP, entityRef, entityId, id, instanceId, createdAt, modifiedAt, unitCode);
  else if (valueNodeP->type == KjInt)
    pgIntPropertyValueTreat(connectionP, valueNodeP, entityRef, entityId, id, instanceId, createdAt, modifiedAt, unitCode);
  else if (valueNodeP->type == KjBool)
    pgBoolPropertyValueTreat(connectionP, valueNodeP, entityRef, entityId, id, instanceId, createdAt, modifiedAt, unitCode);
#endif
  else
    return true;

  return true;
}
