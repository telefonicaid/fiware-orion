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
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/troe.h"                                 // TroeMode, troeMode
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAttributeBuild.h"                     // pgAttributeBuild
#include "orionld/troe/pgSubAttributeBuild.h"                  // pgSubAttributeBuild
#include "orionld/troe/pgEntityBuild.h"                        // Own interface



// ----------------------------------------------------------------------------
//
// entityAppend -
//
// INSERT INTO entities(instanceId,
//                      ts,
//                      opMode,
//                      id,
//                      type) VALUES (), (), ...
//
static void entityAppend(PgAppendBuffer* entitiesBufferP, const char* opMode, const char* entityId, const char* entityType, const char* instanceId)
{
  char         buf[1024];
  const char*  comma = (entitiesBufferP->values != 0)? "," : "";

  snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', '%s')", comma, instanceId, orionldState.requestTimeString, opMode, entityId, entityType);

  LM_TMP(("Calling pgAppend"));
  pgAppend(entitiesBufferP, buf, 0);
  LM_TMP(("Back from pgAppend"));
  entitiesBufferP->values += 1;
  LM_TMP(("Here"));
}



// ----------------------------------------------------------------------------
//
// pgEntityBuild -
//
bool pgEntityBuild
(
  PgAppendBuffer*  entitiesBufferP,
  const char*      opMode,
  KjNode*          entityNodeP,
  char*            entityId,
  char*            entityType,
  PgAppendBuffer*  attributesBufferP,
  PgAppendBuffer*  subAttributesBufferP
)
{
  char instanceId[80];

  uuidGenerate(instanceId, sizeof(instanceId), true);

  //
  // If entity ID and TYPE still in there - remove them!
  //
  if (entityId == NULL)
  {
    KjNode* entityIdNodeP = kjLookup(entityNodeP, "id");

    if (entityIdNodeP == NULL)
      LM_RE(false, ("entity without id"));

    entityId = entityIdNodeP->value.s;
    kjChildRemove(entityNodeP, entityIdNodeP);
  }

  if (entityType == NULL)
  {
    KjNode* entityTypeNodeP = kjLookup(entityNodeP, "type");

    if (entityTypeNodeP == NULL)
      LM_RE(false, ("entity without type"));

    entityType = entityTypeNodeP->value.s;
    kjChildRemove(entityNodeP, entityTypeNodeP);
  }

  if ((entityId == NULL) || (entityType == NULL))
    LM_RE(false, ("Missing Entity id/type"));

  // We have all the entity info - time to add the entity
  LM_TMP(("Entity SQL buffer BEFORE: '%s'", entitiesBufferP->buf));
  entityAppend(entitiesBufferP, opMode, entityId, entityType, instanceId);
  LM_TMP(("Entity SQL buffer AFTER: '%s'", entitiesBufferP->buf));

  // All that remains in 'entityNodeP' are attributes!
  for (KjNode* attrP = entityNodeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type == KjArray)
    {
      for (KjNode* aiP = attrP->value.firstChildP; aiP != NULL; aiP = aiP->next)
      {
        aiP->name = attrP->name;
        pgAttributeBuild(attributesBufferP, opMode, entityId, aiP, subAttributesBufferP);
      }
    }
    else
      pgAttributeBuild(attributesBufferP, opMode, entityId, attrP, subAttributesBufferP);
  }

  return true;
}
