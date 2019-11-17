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
#include <unistd.h>                                              // NULL

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem



// ----------------------------------------------------------------------------
//
// orionldContextItemValueLookup - lookup a value in a context
//
OrionldContextItem* orionldContextItemValueLookup(OrionldContext* contextP, const char* longname)
{
  OrionldContextItem* itemP = NULL;

  LM_TMP(("ALIAS: Looking for value '%s' in context '%s'", longname, contextP->url));

  if (contextP->keyValues == true)
  {
    LM_TMP(("ALIAS: Context is an Object (Hashed key-values): calling khashItemLookup"));
    itemP = (OrionldContextItem*) khashItemLookup(contextP->context.hash.valueHashTable, longname);
    if (itemP != NULL)
      LM_TMP(("ALIAS: Found %s: %s", longname, itemP->name));
  }
  else
  {
    LM_TMP(("ALIAS: Context is an Array"));
    for (int ix = 0; ix < contextP->context.array.items; ++ix)
    {
      LM_TMP(("ALIAS: Recursive call to orionldContextItemValueLookup for context '%s'", contextP->context.array.vector[ix]->url));
      if ((itemP = orionldContextItemValueLookup(contextP->context.array.vector[ix], longname)) != NULL)
      {
        LM_TMP(("ALIAS: Found %s: %s", longname, itemP->name));
        break;
      }
    }
  }

  return itemP;
}
