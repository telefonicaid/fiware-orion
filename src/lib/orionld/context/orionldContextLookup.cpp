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

#include "orionld/context/OrionldContext.h"                 // OrionldContext
#include "orionld/context/orionldContextList.h"             // orionldContextHead



// -----------------------------------------------------------------------------
//
// orionldContextLookup -
//
OrionldContext* orionldContextLookup(const char* url)
{
  OrionldContext* contextP = orionldContextHead;

  LM_T(LmtContext, ("Looking up context '%s'", url));
  LM_TMP(("Looking up context '%s'", url));

  while (contextP != NULL)
  {
    LM_T(LmtContextLookup, ("Comparing '%s' with '%s'", contextP->url, url));
    if (strcmp(contextP->url, url) == 0)
    {
      LM_T(LmtContextLookup, ("Found it!"));
      LM_TMP(("Found context '%s'", url));
      return contextP;
    }

    contextP = contextP->next;
    LM_T(LmtContextLookup, ("No match. Next context at %p", contextP));
  }

  LM_T(LmtContextLookup, ("NOT Found"));
  LM_TMP(("Can't find context '%s'", url));
  return NULL;
}  
