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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate

#include "orionld/temporal/temporal.h"                         // TemporalMode
#include "orionld/temporal/pgEntityPush.h"                     // pgEntityPush
#include "orionld/temporal/pgAttributeTreat.h"                 // pgAttributeTreat
#include "orionld/temporal/pgEntityTreat.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pgEntityTreat -
//
bool pgEntityTreat(PGconn* connectionP, KjNode* entityP, char* id, char* type, char* createdAt, char* modifiedAt, TemporalMode opMode)
{
  char  entityInstance[64];
  char* entityInstanceP = NULL;

  // <DEBUG>
  char buf[1024];
  kjRender(orionldState.kjsonP, entityP, buf, sizeof(buf));
  LM_TMP(("TEMP: entityP: %s", buf));
  // </DEBUG>

  if (id == NULL)  // Find the entity id in the entity tree
  {
    KjNode* nodeP = kjLookup(entityP, "id");

    if (nodeP == NULL)
      LM_X(1, ("Entity without id"));

    id = nodeP->value.s;
    kjChildRemove(entityP, nodeP);
  }


  if (type == NULL)  // Find the entity type in the entity tree
  {
    KjNode* nodeP = kjLookup(entityP, "type");

    if (nodeP == NULL)
      LM_X(1, ("Entity without type"));

    type = nodeP->value.s;
    kjChildRemove(entityP, nodeP);
  }

  if (opMode == TEMPORAL_ENTITY_CREATE)
  {
    uuidGenerate(entityInstance);
    entityInstanceP = entityInstance;
    LM_TMP(("Calling pgEntityPush(%p, '%s', '%s', '%s', '%s', '%s')", connectionP, entityInstanceP, id, type, createdAt, modifiedAt));
    if (pgEntityPush(connectionP, entityInstanceP, id, type, createdAt, modifiedAt, "Create") == false)
      LM_RE(false, ("pgEntityPush failed"));
  }

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type == KjObject)
    {
      LM_TMP(("APPA: Calling pgAttributeTreat with opMode %s", temporalMode(opMode)));
      if (pgAttributeTreat(connectionP, attrP, entityInstanceP, id, createdAt, modifiedAt, opMode) == false)
        LM_RE(false, ("pgAttributeTreat failed for attribute '%s'", attrP->name));
    }
    else if (attrP->type == KjArray)
    {
      LM_W(("The attribute '%s' is an array ... datasetId is not supported for TRoE", attrP->name));
    }
  }

  return true;
}
