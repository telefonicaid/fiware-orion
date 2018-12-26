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

  LM_TMP(("----- In orionldContextLookup"));

  LM_TMP(("orionldContextLookup: url: '%s'", url));
  if (contextP != NULL)
    LM_TMP(("orionldContextLookup: contextP->url: '%s'", contextP->url));
  else
    LM_TMP(("orionldContextLookup: NULL contextP"));

  int contextIx = 0;

  while (contextP != NULL)
  {
    LM_TMP(("contextP: %p", contextP));

    if (contextP->url == NULL)
    {
      LM_E(("KZ: This seems like a bug. Every context should have a URL ..."));
      LM_E(("KZ: tree at %p", contextP->tree));
      LM_E(("KZ: type: %d", contextP->type));
      LM_E(("KZ: ignore: %s", FT(contextP->ignore)));
      LM_E(("KZ: next at %p", contextP->next));
      return NULL;
    }
    
    LM_T(LmtContextLookup, ("Comparing context %d: '%s' with '%s'", contextIx, contextP->url, url));
    LM_TMP(("Comparing '%s' with '%s'", contextP->url, url));
    if (strcmp(contextP->url, url) == 0)
    {
      LM_T(LmtContextLookup, ("Found it!"));
      LM_TMP(("Found it!"));
      return contextP;
    }

    contextP = contextP->next;
    LM_T(LmtContextLookup, ("No match. Next context at %p", contextP));
    LM_TMP(("No match. Next context at %p", contextP));
    ++contextIx;
  }

  LM_T(LmtContextLookup, ("NOT Found"));
  LM_TMP(("NOT Found"));

  return NULL;
}  
