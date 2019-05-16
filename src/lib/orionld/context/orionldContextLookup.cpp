/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                  // LM_*
#include "logMsg/traceLevels.h"                             // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                   // KjNode
}

#include "common/string.h"                                  // FT
#include "orionld/context/OrionldContext.h"                 // OrionldContext
#include "orionld/context/orionldContextList.h"             // orionldContextHead



// -----------------------------------------------------------------------------
//
// orionldContextLookup -
//
OrionldContext* orionldContextLookup(const char* url)
{
  OrionldContext* contextP = orionldContextHead;

  LM_T(LmtContextLookup, ("----- In orionldContextLookup. Looking for context '%s'", url));

  int contextIx = 0;

  orionldContextListSemTake("Looking up a context");
  while (contextP != NULL)
  {
    if (contextP->url == NULL)
    {
      LM_E(("KZ: This seems like a bug. Every context should have a URL ..."));
      LM_E(("KZ: tree at %p", contextP->tree));
      LM_E(("KZ: type: %d", contextP->type));
      LM_E(("KZ: ignore: %s", FT(contextP->ignore)));
      LM_E(("KZ: next at %p", contextP->next));
      orionldContextListSemGive("Looking up a context - no luck");
      return NULL;
    }

    LM_T(LmtContextLookup, ("Comparing context %d: '%s' with '%s'", contextIx, contextP->url, url));
    if (strcmp(contextP->url, url) == 0)
    {
      LM_T(LmtContextLookup, ("Found the context '%s'", url));
      orionldContextListSemGive("Looking up a context - Found it");
      return contextP;
    }

    contextP = contextP->next;
    LM_T(LmtContextLookup, ("No match. Next context at %p", contextP));
    ++contextIx;
  }

  LM_T(LmtContextLookup, ("NOT Found"));

  orionldContextListSemGive("Looking up a context - NOT Found");
  return NULL;
}
