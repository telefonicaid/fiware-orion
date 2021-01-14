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

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/pgStringPropertyPush.h"                 // pgStringPropertyPush
#include "orionld/troe/pgNumberPropertyPush.h"                 // pgNumberPropertyPush
#include "orionld/troe/pgCompoundPropertyPush.h"               // pgCompoundPropertyPush
#include "orionld/troe/pgBoolPropertyPush.h"                   // pgBoolPropertyPush
#include "orionld/troe/pgRelationshipPush.h"                   // pgRelationshipPush
#include "orionld/troe/pgGeoPropertyPush.h"                    // pgGeoPropertyPush
#include "orionld/troe/pgAttributePush.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// pgAttributePush - add an attribute to the DB
//
bool pgAttributePush
(
  PGconn*      connectionP,
  KjNode*      valueNodeP,
  const char*  attributeType,
  const char*  entityId,
  const char*  id,
  const char*  instanceId,
  const char*  datasetId,
  const char*  observedAt,
  bool         subAttrs,
  const char*  unitCode,
  const char*  opMode
)
{
  LM_TMP(("OBS: In pgAttributePush with observedAt: %s", observedAt));
  //
  // Property
  //
  if (strcmp(attributeType, "Property") == 0)
  {
    if (valueNodeP->type == KjString)
    {
      LM_TMP(("OBS: Calling pgStringPropertyPush with observedAt: %s", observedAt));
      if (pgStringPropertyPush(connectionP, opMode, valueNodeP->value.s, entityId, id, instanceId, datasetId, observedAt, subAttrs) == false)
        LM_RE(false, ("pgStringPropertyPush failed"));
    }
    else if (valueNodeP->type == KjInt)
    {
      if (pgNumberPropertyPush(connectionP, opMode, valueNodeP->value.i, entityId, id, instanceId, datasetId, observedAt, subAttrs, unitCode) == false)
        LM_RE(false, ("pgIntPropertyPush failed"));
    }
    else if (valueNodeP->type == KjFloat)
    {
      if (pgNumberPropertyPush(connectionP, opMode, valueNodeP->value.f, entityId, id, instanceId, datasetId, observedAt, subAttrs, unitCode) == false)
        LM_RE(false, ("pgIntPropertyPush failed"));
    }
    else if ((valueNodeP->type == KjArray) || (valueNodeP->type == KjObject))
    {
      if (pgCompoundPropertyPush(connectionP, opMode, valueNodeP, entityId, id, instanceId, datasetId, observedAt, subAttrs) == false)
        LM_RE(false, ("pgCompoundPropertyPush failed"));
    }
    else if (valueNodeP->type == KjBoolean)
    {
      if (pgBoolPropertyPush(connectionP, opMode, valueNodeP->value.b, entityId, id, instanceId, datasetId, observedAt, subAttrs) == false)
        LM_RE(false, ("pgBoolPropertyPush failed"));
    }
    else
    {
      LM_E(("Internal Error (invalid value type for the Property '%s')", id));
      return false;
    }
  }
  //
  // Relationship
  //
  else if (strcmp(attributeType, "Relationship") == 0)
  {
    if (pgRelationshipPush(connectionP, opMode, valueNodeP->value.s, entityId, id, instanceId, datasetId, observedAt, subAttrs) == false)
      LM_RE(false, ("pgRelationshipPush failed"));
  }
  //
  // GeoProperty
  //
  else if (strcmp(attributeType, "GeoProperty") == 0)
  {
    if (pgGeoPropertyPush(connectionP, opMode, valueNodeP, entityId, id, instanceId, datasetId, observedAt, subAttrs) == false)
      LM_RE(false, ("pgGeoPropertyPush failed"));
  }
  else
  {
    LM_E(("Internal Error (invalid type (%s) for the attribute '%s')", attributeType, id));
    return false;
  }

  return true;
}
