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
#include "kjson/kjFree.h"                                   // kjFree
}

#include "orionld/context/OrionldContext.h"                 // OrionldContext
#include "orionld/context/orionldContextList.h"             // orionldContextHead
#include "orionld/context/orionldDefaultContext.h"          // orionldDefaultContext
#include "orionld/context/orionldContextFree.h"             // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextFreeAll -
//
void orionldContextFreeAll(void)
{
  OrionldContext* contextP = orionldContextHead;

  LM_T(LmtFree, ("Freeing contexts, starting with %p", contextP));
  LM_TMP(("Freeing contexts, starting with %p", contextP));
  while (contextP != NULL)
  {
    OrionldContext* next = contextP->next;

    LM_T(LmtContextList, ("Freeing context '%s' at %p", contextP->url, contextP));
    LM_TMP(("Freeing context '%s' at %p", contextP->url, contextP));
    kjFree(contextP->tree);

    if (contextP != &orionldDefaultContext)
    {
      free(contextP->url);
      free(contextP);
    }

    contextP = next;
  }

  LM_T(LmtFree, ("Freed all context"));
}
