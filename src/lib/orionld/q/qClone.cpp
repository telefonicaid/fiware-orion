/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                            // strdup
#include <unistd.h>                                            // NULL

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/q/QNode.h"                                   // QNode, qNode
#include "orionld/q/qClone.h"                                  // Own interface



// -----------------------------------------------------------------------------
//
// qClone - clone QNode tree
//
QNode* qClone(QNode* original)
{
  QNode* cloneP = qNode(original->type);

  if (orionldState.useMalloc == true)
  {
    if (original->type == QNodeVariable)
    {
      LM_TMP(("LEAK: cloning Vatiable '%s' at %p", original->value.v, original->value.v));
      cloneP->value.v = strdup(original->value.v);
      LM_TMP(("LEAK: cloned VariablePath '%s' at %p for qNode at %p", cloneP->value.v, cloneP->value.v, cloneP));
    }
    else if (original->type == QNodeStringValue)
    {
      cloneP->value.s = strdup(original->value.s);
      LM_TMP(("strdupped String '%s' at %p for qNode at %p", cloneP->value.s, cloneP->value.s, cloneP));
    }
    else
      cloneP->value = original->value;
  }
  else
    cloneP->value = original->value;

  cloneP->next = NULL;

  return cloneP;
}
