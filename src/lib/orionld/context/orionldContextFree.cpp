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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/kjFree.h"                                      // kjFree
}

#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextFree.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextFree -
//
void orionldContextFree(OrionldContext* contextP)
{
  if (contextP == NULL)
  {
    LM_TMP(("NOT Freeing LIST context (NULL)"));
    return;
  }
  
  LM_TMP(("Freeing LIST context '%s' at %p, tree at %p", contextP->url, contextP, contextP->tree));

  if (contextP->tree != NULL)
  {
    kjFree(contextP->tree);
    contextP->tree = NULL;
  }

  if (contextP->type == OrionldUserContext)
  {
    if (contextP->url != NULL)
      free(contextP->url);
    free(contextP);
  }
}
