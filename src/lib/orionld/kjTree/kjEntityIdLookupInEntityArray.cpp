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
#include <string.h>                                                        // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                                  // KjNode
#include "kjson/kjLookup.h"                                                // kjLookup
}

#include "orionld/kjTree/kjEntityIdLookupInEntityArray.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// kjEntityIdLookupInEntityArray -
//
// NOTE
//   This lookup function works for Entity Arrays (batch operations)
//   For each item in the array, a field called "id" is looked up and compared to 'value'
//
KjNode* kjEntityIdLookupInEntityArray(KjNode* entityArrayP, const char* value)
{
  if (entityArrayP == NULL)
    return NULL;

  if (entityArrayP->type != KjArray)
    return NULL;

  for (KjNode* entityP = entityArrayP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (entityP->type != KjObject)
      continue;

    KjNode* idP = kjLookup(entityP, "id");

    if (idP == NULL)
      idP = kjLookup(entityP, "@id");

    if ((idP != NULL) && (strcmp(value, idP->value.s) == 0))
      return entityP;
  }

  return NULL;
}
