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
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "khash/khash.h"                                         // KHashTable, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/orionldContextCache.h"                 // Context Cache Internals
#include "orionld/context/orionldContextCacheGet.h"              // Own interface


extern const char* originName(OrionldContextOrigin origin);  // FIXME: move to own module
// -----------------------------------------------------------------------------
//
// orionldContextCacheGet -
//
KjNode* orionldContextCacheGet(KjNode* arrayP, bool details)
{
  for (int ix = 0; ix < orionldContextCacheSlotIx; ix++)
  {
    OrionldContext*  contextP         = orionldContextCacheArray[ix];

    if (contextP == NULL)
      continue;

    if (details == true)
    {
      KjNode*          contextObjP      = kjObject(orionldState.kjsonP, NULL);
      KjNode*          urlStringP       = kjString(orionldState.kjsonP, "url",    contextP->url);
      KjNode*          idStringP        = kjString(orionldState.kjsonP, "id",     (contextP->id == NULL)? "None" : contextP->id);
      KjNode*          typeStringP      = kjString(orionldState.kjsonP, "type",   contextP->keyValues? "hash-table" : "array");
      KjNode*          originP          = kjString(orionldState.kjsonP, "origin", originName(contextP->origin));

      kjChildAdd(contextObjP, urlStringP);
      kjChildAdd(contextObjP, idStringP);
      kjChildAdd(contextObjP, typeStringP);
      kjChildAdd(contextObjP, originP);

      if (contextP->parent != NULL)
      {
        KjNode* parentP = kjString(orionldState.kjsonP, "parent", contextP->parent);

        kjChildAdd(contextObjP, parentP);
      }

      if (contextP->keyValues)
      {
        // Show a maximum of 5 items from the hash-table
        KjNode*      hashTableObjectP = kjObject(orionldState.kjsonP, "hash-table");
        KHashTable*  htP              = contextP->context.hash.nameHashTable;
        int          noOfItems        = 0;

        for (int slot = 0; slot < ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE; ++slot)
        {
          KHashListItem* itemP = htP->array[slot];

          while (itemP != 0)
          {
            OrionldContextItem* hashItemP       = (OrionldContextItem*) itemP->data;
            KjNode*             hashItemStringP = kjString(orionldState.kjsonP, hashItemP->name, hashItemP->id);

            kjChildAdd(hashTableObjectP, hashItemStringP);

            ++noOfItems;
            if (noOfItems >= 5)
              break;

            itemP = itemP->next;
          }

          if (noOfItems >= 5)
            break;
        }

        kjChildAdd(contextObjP, hashTableObjectP);
      }
      else
      {
        //
        // If ARRAY - show all the URLs in the array
        //
        KjNode* urlArrayP = kjArray(orionldState.kjsonP, "URLs");

        for (int aIx = 0; aIx < contextP->context.array.items; ++aIx)
        {
          KjNode* urlStringP = kjString(orionldState.kjsonP, NULL, contextP->context.array.vector[aIx]->url);

          kjChildAdd(urlArrayP, urlStringP);
        }
        kjChildAdd(contextObjP, urlArrayP);
      }

      kjChildAdd(arrayP, contextObjP);
    }
    else
    {
      KjNode* urlNodeP = kjString(orionldState.kjsonP, NULL, contextP->url);
      kjChildAdd(arrayP, urlNodeP);
    }
  }

  return arrayP;
}
