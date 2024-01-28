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
#include "kjson/kjBuilder.h"                                   // kjChildRemove, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/PgAppendBuffer.h"                      // PgAppendBuffer
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/pgAttributeAppend.h"                    // pgAttributeAppend
#include "orionld/troe/pgSubAttributeBuild.h"                  // pgSubAttributeBuild
#include "orionld/troe/pgObservedAtExtract.h"                  // pgObservedAtExtract
#include "orionld/troe/pgAttributeBuild.h"                     // Own interface



// ----------------------------------------------------------------------------
//
// pgAttributeBuild -
//
bool pgAttributeBuild
(
  PgAppendBuffer*  attributesBuffer,
  const char*      opMode,
  const char*      entityId,
  KjNode*          attributeNodeP,
  PgAppendBuffer*  subAttributesBuffer
)
{
  if (attributeNodeP->type != KjObject)
    return false;

  char    instanceId[80];
  char*   type          = NULL;
  char*   observedAt    = NULL;
  bool    subProperties = false;
  char*   unitCode      = NULL;
  char*   datasetId     = NULL;
  KjNode* valueNodeP    = NULL;
  KjNode* subAttrV      = kjArray(orionldState.kjsonP, NULL);

  uuidGenerate(instanceId, sizeof(instanceId), "urn:ngsi-ld:attribute:instance:");

  // Extract attribute info, call subAttributeBuild when necessary
  KjNode* nodeP = attributeNodeP->value.firstChildP;
  KjNode* next;

  char* skip = NULL;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    if      (strcmp(nodeP->name, "observedAt")  == 0)  observedAt = pgObservedAtExtract(nodeP);
    else if (strcmp(nodeP->name, "unitCode")    == 0)  unitCode   = nodeP->value.s;
    else if (strcmp(nodeP->name, "type")        == 0)  type       = nodeP->value.s;
    else if (strcmp(nodeP->name, "datasetId")   == 0)  datasetId  = nodeP->value.s;
    else if (strcmp(nodeP->name, "value")       == 0)  valueNodeP = nodeP;
    else if (strcmp(nodeP->name, "object")      == 0)
    {
      if (nodeP->type == KjString)
        valueNodeP = nodeP;
      else
        skip = (char*) "RelationshipArray";
    }
    else if (strcmp(nodeP->name, "languageMap") == 0)  skip = (char*) "LanguageProperty";
    else if (strcmp(nodeP->name, "vocab")       == 0)  skip = (char*) "VocabularyProperty";
    else if (strcmp(nodeP->name, "createdAt")   == 0)  {}  // skip = (char*) "BuiltinTimestamp";
    else if (strcmp(nodeP->name, "modifiedAt")  == 0)  {}  // skip = (char*) "BuiltinTimestamp";
    else if (strcmp(nodeP->name, "https://uri.etsi.org/ngsi-ld/createdAt")  == 0)  {}  // Skipping
    else if (strcmp(nodeP->name, "https://uri.etsi.org/ngsi-ld/modifiedAt") == 0)  {}  // Skipping
    else
    {
      subProperties = true;
      kjChildRemove(attributeNodeP, nodeP);
      kjChildAdd(subAttrV, nodeP);
    }

    nodeP = next;
  }

  if (skip != NULL)
  {
    LM_W(("Sorry, attributes of type '%s' are not stored in the Temporal database right now (to be implemented)", skip));
    return true;
  }

  pgAttributeAppend(attributesBuffer, instanceId, attributeNodeP->name, opMode, entityId, type, observedAt, subProperties, unitCode, datasetId, valueNodeP);

  // Now that all the attribute data is gathered, we can serve the sub-attrs
  for (nodeP = subAttrV->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    pgSubAttributeBuild(subAttributesBuffer,
                        entityId,
                        instanceId,
                        datasetId,
                        nodeP);
  }

  return true;
}
