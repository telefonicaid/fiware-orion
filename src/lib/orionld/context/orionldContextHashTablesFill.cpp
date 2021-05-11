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

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "khash/khash.h"                                         // KHashTable, KHashListItem, khashItemAdd, ...
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/common/orionldState.h"                         // orionldState, kalloc
#include "orionld/context/OrionldContext.h"                      // OrionldContext, OrionldContextHashTables
#include "orionld/contextCache/orionldContextCache.h"            // ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/orionldContextPrefixExpand.h"          // orionldContextPrefixExpand
#include "orionld/context/orionldContextHashTablesFill.h"        // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextHashTablesFill -
//
bool orionldContextHashTablesFill(OrionldContext* contextP, KjNode* keyValueTree, OrionldProblemDetails* pdP)
{
  OrionldContextHashTables* hashP           = &contextP->context.hash;
  KHashTable*               nameHashTableP  = hashP->nameHashTable;
  KHashTable*               valueHashTableP = hashP->valueHashTable;

  for (KjNode* kvP = keyValueTree->value.firstChildP; kvP != NULL; kvP = kvP->next)
  {
    OrionldContextItem* hiP = (OrionldContextItem*) kaAlloc(&kalloc, sizeof(OrionldContextItem));

    hiP->name = kaStrdup(&kalloc, kvP->name);
    hiP->type = NULL;

    if (kvP->type == KjString)
      hiP->id = kvP->value.s;  // Will be allocated in pass II
    else if (kvP->type == KjObject)
    {
      hiP->id = NULL;

      //
      // Find @id, @type
      //
      for (KjNode* itemP = kvP->value.firstChildP; itemP != NULL; itemP = itemP->next)
      {
        if (strcmp(itemP->name, "@id") == 0)
          hiP->id = itemP->value.s;  // Will be allocated in pass II
        else if (strcmp(itemP->name, "@type") == 0)
          hiP->type = kaStrdup(&kalloc, itemP->value.s);
      }
    }
    else
    {
      LM_W(("Bad Input (invalid value type for '%s': %s)", kvP->name, kjValueType(kvP->type)));
      orionldProblemDetailsFill(pdP, OrionldBadRequestData, "Invalid key-value in @context", kvP->name, 400);
      return false;
    }

    if ((hiP->id == NULL) || (hiP->id[0] == 0))
    {
      LM_W(("Bad Input (NULL value for key '%s')", kvP->name));

      pdP->type   = OrionldBadRequestData;
      pdP->title  = (char*) "NULL value for key in context";
      pdP->detail = (char*) kvP->name;
      pdP->status = 400;

      return false;
    }

    khashItemAdd(nameHashTableP,  hiP->name, hiP);
  }


  //
  // Second pass, to fix prefix expansion in the values, and to create the valueHashTable
  // In this pass, the 'id' (value) is allocated on the global kalloc instance
  //
  for (int slot = 0; slot < ORIONLD_CONTEXT_CACHE_HASH_ARRAY_SIZE; ++slot)
  {
    KHashListItem* itemP = nameHashTableP->array[slot];

    while (itemP != NULL)
    {
      //
      // Expand if the value contains a colon
      //
      OrionldContextItem* hashItemP = (OrionldContextItem*) itemP->data;
      char*               colonP    = strchr(hashItemP->id, ':');

      if (colonP != NULL)
        hashItemP->id = orionldContextPrefixExpand(contextP, hashItemP->id, colonP);

      hashItemP->id = kaStrdup(&kalloc, hashItemP->id);
      khashItemAdd(valueHashTableP, hashItemP->id, hashItemP);

      itemP = itemP->next;
    }
  }

  return true;
}
