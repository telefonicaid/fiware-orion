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
#include <string.h>                                              // strchr

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextPrefixExpand.h"          // orionldContextPrefixExpand
#include "orionld/context/orionldContextItemLookup.h"            // orionldContextItemLookup
#include "orionld/context/orionldContextItemExpand.h"            // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextItemExpand -
//
// PARAMETERS
//   contextP                the context
//   shortName               the name to expand
//   contextItemPP           to give the caller the complete result of the lookup
//
// RETURN VALUE
//   orionldContextItemExpand returns a pointer to the expanded value of 'shortName'
//
// NOTE
//   If no expansion is found, and the default URL has been used, then room is allocated using
//   kaAlloc, allocating on orionldState.kalloc, the connection buffer that lives only during
//   the current request. It is liberated "automatically" when the thread exits.
//
//   If the expansion IS found, then a pointer to the longname (that is part of the context where it was found)
//   is returned and we save some time by not copying anything.
//
char* orionldContextItemExpand
(
  OrionldContext*       contextP,
  const char*           shortName,
  bool                  useDefaultUrlIfNotFound,
  OrionldContextItem**  contextItemPP
)
{
  OrionldContextItem* contextItemP;
  char*               colonP;

  if (contextP == NULL)
    contextP = orionldCoreContextP;

  if ((colonP = strchr((char*) shortName, ':')) != NULL)
    return orionldContextPrefixExpand(contextP, shortName, colonP);

  // 1. Lookup in Core Context
  contextItemP = orionldContextItemLookup(orionldCoreContextP, shortName, NULL);

  // 2. Lookup in given context (unless it's the Core Context)
  if ((contextItemP == NULL) && (contextP != orionldCoreContextP))
    contextItemP = orionldContextItemLookup(contextP, shortName, NULL);

  // 3. Use the Default URL (or not!)
  if (contextItemP == NULL)
  {
    if (useDefaultUrlIfNotFound == true)
    {
      int   shortNameLen = strlen(shortName);
      int   longNameLen  = orionldDefaultUrlLen + shortNameLen + 1;
      char* longName     = (char*) kaAlloc(&orionldState.kalloc, longNameLen);

      snprintf(longName, longNameLen, "%s%s", orionldDefaultUrl, shortName);

      if (contextItemPP != NULL)
        *contextItemPP = NULL;

      return longName;
    }

    return NULL;
  }

  // 4. Save the pointer to the context item
  if (contextItemPP != NULL)
    *contextItemPP = contextItemP;

  return contextItemP->id;
}
