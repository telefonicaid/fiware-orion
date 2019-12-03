/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, kalloc
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // Own interface



// -----------------------------------------------------------------------------
//
// valueExpand -
//
static void valueExpand(KjNode* nodeP)
{
  nodeP->value.s = orionldContextItemExpand(orionldState.contextP, nodeP->value.s, NULL, true, NULL);
}



// -----------------------------------------------------------------------------
//
// orionldContextValueExpand -
//
// The "value" is only expanded if the type of the value is either KjString or KjArray
//
void orionldContextValueExpand(KjNode* attrNodeP)
{
  KjNode* valueNodeP = kjLookup(attrNodeP, "value");

  if (valueNodeP == NULL)
    return;

  if (valueNodeP->type == KjArray)
  {
    for (KjNode* nodeP = valueNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (nodeP->type == KjString)
        valueExpand(nodeP);
    }
  }
  else if (valueNodeP->type == KjString)
    valueExpand(valueNodeP);
}
