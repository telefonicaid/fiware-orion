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
#include "khash/khash.h"                                         // KHashTable
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/orionldContextCache.h"                 // ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE
#include "orionld/context/orionldContextPresent.h"               // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextPresent -
//
void orionldContextPresent(const char* prefix, OrionldContext* contextP)
{
  if (contextP == NULL)
    return;

  LM_K(("    %s: Context '%s' (%s)", prefix, contextP->url, contextP->keyValues? "Key-Values" : "Array"));
  LM_K(("    %s: ----------------------------------------------------------------------------", prefix));

  if (contextP->keyValues == true)
  {
    int          noOfItems = 0;
    KHashTable*  htP       = contextP->context.hash.nameHashTable;

    for (int slot = 0; slot < ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE; slot++)
    {
      KHashListItem* itemP = htP->array[slot];

      while (itemP != 0)
      {
        OrionldContextItem* hiP = (OrionldContextItem*) itemP->data;

        LM_K(("    %s: key-value[slot %d]: %s -> %s (type: %s)", prefix, slot, hiP->name, hiP->id, hiP->type));
        itemP = itemP->next;
        ++noOfItems;

        if (noOfItems >= 100)
          break;
      }

      if (noOfItems >= 100)
        break;
    }
  }
  else
  {
    for (int iIx = 0; iIx < contextP->context.array.items; iIx++)
    {
      if (contextP->context.array.vector[iIx] == NULL)
      {
        LM_K(("    %s:   Array Item %d is not ready", prefix, iIx));
      }
      else
      {
        LM_K(("    %s:   Array Item %d: %s (%s)",
              prefix,
              iIx,
              contextP->context.array.vector[iIx]->url,
              contextP->context.array.vector[iIx]->keyValues? "Key-Values" : "Array"));
      }
    }
  }
}
