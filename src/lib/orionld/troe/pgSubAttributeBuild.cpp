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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/PgAppendBuffer.h"                      // PgAppendBuffer
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/pgObservedAtExtract.h"                  // pgObservedAtExtract
#include "orionld/troe/pgSubAttributeAppend.h"                 // pgSubAttributeAppend
#include "orionld/troe/pgSubAttributeBuild.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// pgSubAttributeBuild -
//
bool pgSubAttributeBuild
(
  PgAppendBuffer*  subAttributesBuffer,
  const char*      entityId,
  char*            attrInstanceId,
  char*            attrDatasetId,
  KjNode*          subAttributeNodeP
)
{
  char    instanceId[80];
  char*   type          = NULL;
  char*   observedAt    = NULL;
  char*   unitCode      = NULL;
  char*   object        = NULL;  // Only for relationships
  KjNode* valueNodeP    = NULL;

  if (subAttributeNodeP->type != KjObject)
    return true;

  uuidGenerate(instanceId, sizeof(instanceId), "urn:ngsi-ld:attribute:instance:");

  // Extract sub-attribute info
  for (KjNode* nodeP = subAttributeNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if      (strcmp(nodeP->name, "observedAt") == 0)  observedAt = pgObservedAtExtract(nodeP);
    else if (strcmp(nodeP->name, "unitCode")   == 0)  unitCode   = nodeP->value.s;
    else if (strcmp(nodeP->name, "type")       == 0)  type       = nodeP->value.s;
    else if (strcmp(nodeP->name, "value")      == 0)  valueNodeP = nodeP;
    else if (strcmp(nodeP->name, "object")     == 0)  object     = nodeP->value.s;
    else if (strcmp(nodeP->name, "createdAt")  == 0)  {}  // Skipping
    else if (strcmp(nodeP->name, "modifiedAt") == 0)  {}  // Skipping
    else
    {
      // Sub-sub-attributes are ignored in TRoE
    }
  }

  pgSubAttributeAppend(subAttributesBuffer, instanceId, subAttributeNodeP->name, entityId, attrInstanceId, attrDatasetId, type, observedAt, unitCode, valueNodeP, object);

  return true;
}
