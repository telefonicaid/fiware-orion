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

#include "orionld/types/OrionldContext.h"                        // OrionldContext, orionldOriginToString, OrionldContextKind
#include "orionld/types/OrionldContextItem.h"                    // OrionldContextItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/contextCache/orionldContextCache.h"            // Context Cache Internals
#include "orionld/contextCache/orionldContextCacheGet.h"         // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextWithDetails - FIXME: rename and put in separate module
//
KjNode* orionldContextWithDetails(OrionldContext* contextP)
{
  char createdAtString[64];
  char lastUseString[64];

  numberToDate(contextP->createdAt, createdAtString, sizeof(createdAtString));
  numberToDate(contextP->usedAt,    lastUseString,   sizeof(lastUseString));

  //
  // Details, according to spec:
  //   URL
  //   localId
  //   kind   (Hosted, Cached, ImplicitlyCreated)
  //   createdAt
  //   lastUsage (optional)
  //   numberOfHits (optional)
  //   extraInfo (object with broker specific fields, like "type", "origin")
  //

  KjNode*          contextObjP      = kjObject(orionldState.kjsonP, NULL);

  KjNode*          urlStringP       = kjString(orionldState.kjsonP,  "URL",          contextP->url);
  KjNode*          idStringP        = kjString(orionldState.kjsonP,  "localId",      (contextP->id == NULL)? "None" : contextP->id);
  KjNode*          kindP            = kjString(orionldState.kjsonP,  "kind",         orionldKindToString(contextP->kind));
  KjNode*          createdAtP       = kjString(orionldState.kjsonP,  "createdAt",    createdAtString);
  KjNode*          extraInfoP       = kjObject(orionldState.kjsonP,  "extraInfo");
  KjNode*          typeStringP      = kjString(orionldState.kjsonP,  "type",         (contextP->keyValues == true)? "hash-table" : "array");
  KjNode*          originP          = kjString(orionldState.kjsonP,  "origin",       orionldOriginToString(contextP->origin));
  KjNode*          compactionsP     = kjInteger(orionldState.kjsonP, "compactions",  contextP->compactions);
  KjNode*          expansionsP      = kjInteger(orionldState.kjsonP, "expansions",   contextP->expansions);

  kjChildAdd(contextObjP, urlStringP);
  kjChildAdd(contextObjP, idStringP);
  kjChildAdd(contextObjP, kindP);
  kjChildAdd(contextObjP, createdAtP);
  kjChildAdd(contextObjP, extraInfoP);


  if (contextP != orionldCoreContextP)
  {
    KjNode*          usedAtP          = kjString(orionldState.kjsonP,  "lastUsage",    lastUseString);
    KjNode*          lookupsP         = kjInteger(orionldState.kjsonP, "numberOfHits", contextP->lookups);
    kjChildAdd(contextObjP, usedAtP);
    kjChildAdd(contextObjP, lookupsP);
  }

  kjChildAdd(extraInfoP,  typeStringP);
  kjChildAdd(extraInfoP,  originP);
  kjChildAdd(extraInfoP,  compactionsP);
  kjChildAdd(extraInfoP,  expansionsP);

  if (contextP->parent != NULL)
  {
    KjNode* parentP = kjString(orionldState.kjsonP, "parent", contextP->parent);
    kjChildAdd(extraInfoP, parentP);
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

    kjChildAdd(extraInfoP, hashTableObjectP);
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
    kjChildAdd(extraInfoP, urlArrayP);
  }

  return contextObjP;
}



// -----------------------------------------------------------------------------
//
// orionldContextCacheGet -
//
KjNode* orionldContextCacheGet(KjNode* arrayP, bool details, OrionldContextKind kind)
{
  for (int ix = 0; ix < orionldContextCacheSlotIx; ix++)
  {
    OrionldContext*  contextP = orionldContextCacheArray[ix];

    if (contextP == NULL)
      continue;

    if ((kind != OrionldContextUnknownKind) && (contextP->kind != kind))
      continue;

    if (details == true)
    {
      KjNode* contextObjP = orionldContextWithDetails(contextP);
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
