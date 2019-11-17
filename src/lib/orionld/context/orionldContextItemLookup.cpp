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
#include <string.h>                                              // strcmp
#include <unistd.h>                                              // NULL

extern "C"
{
#include "khash/khash.h"                                         // KHashTable
}

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemLookup.h"            // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextItemLookup - lookup an item in a context
//
OrionldContextItem* orionldContextItemLookup(OrionldContext* contextP, const char* name, bool* valueMayBeCompactedP)
{
  OrionldContextItem* itemP = NULL;

  if (contextP == NULL)
    contextP = orionldCoreContextP;

  if (contextP->keyValues == true)
    itemP = (OrionldContextItem*) khashItemLookup(contextP->context.hash.nameHashTable, name);
  else
  {
    for (int ix = 0; ix < contextP->context.array.items; ++ix)
    {
      if ((itemP = orionldContextItemLookup(contextP->context.array.vector[ix], name, valueMayBeCompactedP)) != NULL)
        break;
    }
  }

  if (valueMayBeCompactedP != NULL)
  {
    if ((itemP->type != NULL) && (strcmp(itemP->type, "@vocab") == 0))
      *valueMayBeCompactedP = true;
    else
      *valueMayBeCompactedP = false;
  }

  return itemP;
}
