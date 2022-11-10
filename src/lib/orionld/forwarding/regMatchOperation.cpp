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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/forwarding/regMatchOperation.h"                // Own interface



// -----------------------------------------------------------------------------
//
// regMatchOperation -
//
// FIXME: the operations should be a bitmask in RegCacheItem - no kjLookup, no string comparisons
//
bool regMatchOperation(RegCacheItem* regP, const char* op)
{
  KjNode* operations = kjLookup(regP->regTree, "operations");

  if (operations == NULL)
  {
    LM_W(("Registration without operations - that can't be"));
    kjTreeLog(regP->regTree, "Registration in reg-cache");
    return false;
  }

  for (KjNode* operationP = operations->value.firstChildP; operationP != NULL; operationP = operationP->next)
  {
    if (operationP->type != KjString)
      continue;
    if (strcmp(operationP->value.s, op) == 0)
      return true;
  }

  return false;
}
