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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjChildRemove, kjChildAdd, ...
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/troe.h"                                 // TroeMode, troeMode
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgSubAttributeBuild.h"                  // pgSubAttributeBuild
#include "orionld/troe/pgQuotedString.h"                       // pgQuotedString
#include "orionld/troe/pgAttributeBuild.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// attributeAppend -
//
// INSERT INTO attributes(instanceId,
//                        id,
//                        opMode,
//                        entityId,
//                        observedAt,
//                        subProperties,
//                        unitCode,
//                        datasetId,
//                        valueType,
//
//                        text,
//                        boolean,
//                        number,
//                        datetime,
//                        compound,
//                        geoPoint,
//                        geoPolygon,
//                        geoMultiPolygon,
//                        geoLineString,
//                        geoMultiLineString,
//                        ts) VALUES   ('', '', '', ...), ('', '', '', ...), ('', '', '', ...), ...
//
// This function appends a new ('', '', '', ...) for the VALUES of attributesBuffer
// It also calls pgSubAttributeBuild for any sub-attributes
//
static void attributeAppend
(
  PgAppendBuffer*  attributesBufferP,
  const char*      instanceId,
  const char*      attributeName,
  const char*      opMode,
  const char*      entityId,
  const char*      type,
  char*            observedAt,    // Can be NULL
  bool             subProperties,
  char*            unitCode,      // Can be NULL
  char*            datasetId,     // Can be NULL
  KjNode*          valueNodeP,
  const char*      object
)
{
  char        buf[1024];
  const char* comma = (attributesBufferP->values != 0)? "," : "";

  observedAt = (observedAt == NULL)? (char*) "null" : pgQuotedString(observedAt);
  unitCode   = (unitCode   == NULL)? (char*) "null" : pgQuotedString(unitCode);

  if (datasetId == NULL)
    datasetId  = (char*) "None";

  const char* hasSubProperties = (subProperties == true)? "true" : "false";

  if (strcmp(type, "Relationship") == 0)
  {
    LM_TMP(("TROE: Appending 'Relationship' attribute '%s'", attributeName));
    snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Relationship', '%s', null, null, null, null, null, null, null, null, null, '%s')",
             comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, object, orionldState.requestTimeString);
  }
  else if (strcmp(type, "GeoProperty") == 0)
  {
  }
  else  // Property
  {
    if (valueNodeP->type == KjString)
    {
      LM_TMP(("TROE: Appending 'String' attribute '%s'", attributeName));
      snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'String', '%s', null, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.s, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjBoolean)
    {
      LM_TMP(("TROE: Appending 'Boolean' attribute '%s'", attributeName));
      const char* value = (valueNodeP->value.b == true)? "true" : "false";

      snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Bool', null, %s, null, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, value, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjInt)
    {
      LM_TMP(("TROE: Appending 'Integer' attribute '%s'", attributeName));
      snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Number', null, null, %lld, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.i, orionldState.requestTimeString);
    }
    else if (valueNodeP->type == KjFloat)
    {
      LM_TMP(("TROE: Appending 'Float' attribute '%s'", attributeName));
      snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Number', null, null, %f, null, null, null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, valueNodeP->value.f, orionldState.requestTimeString);
    }
    else if ((valueNodeP->type == KjArray) || (valueNodeP->type == KjObject))
    {
      LM_TMP(("TROE: Appending 'Compound' attribute '%s'", attributeName));
      int          renderedValueSize   = 4 * 1024;
      char*        renderedValue       = kaAlloc(&orionldState.kalloc, renderedValueSize);

      kjFastRender(orionldState.kjsonP, valueNodeP, renderedValue, renderedValueSize);

      snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', %s, %s, %s, '%s', 'Number', null, null, null, null, '%s', null, null, null, null, null, '%s')",
               comma, instanceId, attributeName, opMode, entityId, observedAt, hasSubProperties, unitCode, datasetId, renderedValue, orionldState.requestTimeString);
    }
  }

  pgAppend(attributesBufferP, buf, 0);
  attributesBufferP->values += 1;
  LM_TMP(("TROE: values in attribute buffer: %d", attributesBufferP->values));
}



// ----------------------------------------------------------------------------
//
// pgAttributeBuild -
//
bool pgAttributeBuild(PgAppendBuffer* attributesBuffer, const char* opMode, const char* entityId, KjNode* attributeNodeP, PgAppendBuffer* subAttributesBuffer)
{
  char    instanceId[80];
  char*   type          = NULL;
  char*   observedAt    = NULL;
  bool    subProperties = false;
  char*   unitCode      = NULL;
  char*   datasetId     = NULL;
  char*   object        = NULL;  // Only for relationships
  KjNode* valueNodeP    = NULL;
  KjNode* subAttrV      = kjArray(orionldState.kjsonP, NULL);

  LM_TMP(("TROE: Building attribute '%s'", attributeNodeP->name));

  uuidGenerate(instanceId, sizeof(instanceId), true);

  // Extract attribute info, call subAttributeBuild when necessary
  KjNode* nodeP = attributeNodeP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    if      (strcmp(nodeP->name, "observedAt") == 0)  observedAt = nodeP->value.s;  // Might need pgObservedAtExtract()
    else if (strcmp(nodeP->name, "unitCode")   == 0)  unitCode   = nodeP->value.s;
    else if (strcmp(nodeP->name, "type")       == 0)  type       = nodeP->value.s;
    else if (strcmp(nodeP->name, "datasetId")  == 0)  datasetId  = nodeP->value.s;
    else if (strcmp(nodeP->name, "value")      == 0)  valueNodeP = nodeP;
    else if (strcmp(nodeP->name, "object")     == 0)  object     = nodeP->value.s;
    else if (strcmp(nodeP->name, "createdAt")  == 0)  {}  // Skipping
    else if (strcmp(nodeP->name, "modifiedAt") == 0)  {}  // Skipping
    else
    {
      subProperties = true;
      kjChildRemove(attributeNodeP, nodeP);
      kjChildAdd(subAttrV, nodeP);
    }

    nodeP = next;
  }

  attributeAppend(attributesBuffer, instanceId, attributeNodeP->name, opMode, entityId, type, observedAt, subProperties, unitCode, datasetId, valueNodeP, object);

  // Now that all the attribute data is gathered, we can serve the sub-attrs
  for (nodeP = subAttrV->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    LM_TMP(("TROE: Calling pgSubAttributeBuild on '%s'", nodeP->name));
    pgSubAttributeBuild(subAttributesBuffer, entityId, instanceId, datasetId, nodeP);
  }

  return true;
}
