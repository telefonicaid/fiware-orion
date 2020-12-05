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

#include "orionld/temporal/pgEntityPush.h"                     // pgEntityPush
#include "orionld/temporal/pgAttributeTreat.h"                 // pgAttributeTreat
#include "orionld/temporal/pgEntityTreat.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pgEntityTreat -
//
bool pgEntityTreat(PGconn* connectionP, char* id, char* type, KjNode* entityP)
{
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


  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (pgAttributeTreat(connectionP, attrP) == false)
      LM_RE(false, ("pgAttributeTreat failed for attribute '%s'", attrP->name));
  }


  char instanceId[64];
  uuidGenerate(instanceId);


  if (pgEntityPush(connectionP, id, type, instanceId) == false)
    LM_RE(false, ("pgEntityPush failed"));

  return true;
}
