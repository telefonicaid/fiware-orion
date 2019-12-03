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
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP, orionldDefaultUrl, orionldDefaultUrlLen
#include "orionld/context/orionldContextItemValueLookup.h"       // orionldContextItemValueLookup
#include "orionld/context/orionldContextItemAliasLookup.h"       // Own Interface



// -----------------------------------------------------------------------------
//
// orionldContextItemAliasLookup -
//
char* orionldContextItemAliasLookup
(
  OrionldContext*       contextP,
  const char*           longName,
  bool*                 valueMayBeCompactedP,
  OrionldContextItem**  contextItemPP
)
{
  OrionldContextItem* contextItemP;

  // 0. Set output values to false/NULL
  if (valueMayBeCompactedP != NULL)
    *valueMayBeCompactedP = false;

  if (contextItemPP != NULL)
    *contextItemPP = NULL;


  // 1. Is it the default URL?
  if (strncmp(longName, orionldDefaultUrl, orionldDefaultUrlLen) == 0)
    return (char*) &longName[orionldDefaultUrlLen];

  // 2. Found in Core Context?
  contextItemP = orionldContextItemValueLookup(orionldCoreContextP, longName);

  // 3. If not, look in the provided context, unless it's the Core Context
  if ((contextItemP == NULL) && (contextP != orionldCoreContextP))
    contextItemP = orionldContextItemValueLookup(contextP, longName);

  // 4. If not found anywhere - return the long name
  if (contextItemP == NULL)
    return (char*) longName;

  // 5. Can the value be compacted?
  if ((valueMayBeCompactedP != NULL) && (contextItemP->type != NULL))
  {
    if (strcmp(contextItemP->type, "@vocab") == 0)
      *valueMayBeCompactedP = true;
  }


  // 6. Give back the pointer to the contextItem, if asked for
  if (contextItemPP != NULL)
    *contextItemPP = contextItemP;

  // Return the short name
  return contextItemP->name;
}
