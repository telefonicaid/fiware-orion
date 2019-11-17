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

  LM_TMP(("ALIAS: looking for long-name (in values) '%s'", longName));

  // 0. Set output values to false/NULL
  if (valueMayBeCompactedP != NULL)
    *valueMayBeCompactedP = false;

  if (contextItemPP != NULL)
    *contextItemPP = NULL;


  // 1. Is it the default URL?
  LM_TMP(("ALIAS: looking for long-name '%s' - is it the default URL (%s)?", longName, orionldDefaultUrl));
  if (strncmp(longName, orionldDefaultUrl, orionldDefaultUrlLen) == 0)
  {
    LM_TMP(("ALIAS: looking for long-name '%s' - it was the default URL - returning shortname '%s'", longName, &longName[orionldDefaultUrlLen]));
    return (char*) &longName[orionldDefaultUrlLen];
  }

  // 2. Found in Core Context?
  LM_TMP(("ALIAS: looking for long-name '%s' in the core context '%s'", longName, orionldCoreContextP->url));
  contextItemP = orionldContextItemValueLookup(orionldCoreContextP, longName);
  if (contextItemP != NULL)
    LM_TMP(("ALIAS: looking for long-name '%s' - found in the Core Context (shortname: %s)", longName, contextItemP->name));

  // 3. If not, look in the provided context, unless it's the Core Context
  if ((contextItemP == NULL) && (contextP != orionldCoreContextP))
  {
    LM_TMP(("ALIAS: looking for long-name '%s' in user provided context '%s'", longName, contextP->url));
    contextItemP = orionldContextItemValueLookup(contextP, longName);
    if (contextItemP != NULL)
      LM_TMP(("ALIAS: looking for long-name '%s' - found in user provided context '%s' (shortname: %s)", longName, contextP->url, contextItemP->name));
  }

  // 4. If not found anywhere - return the long name
  if (contextItemP == NULL)
  {
    LM_TMP(("ALIAS: looking for long-name '%s' - NOT FOUND - returning the long name", longName));
    return (char*) longName;
  }

  LM_TMP(("ALIAS: looking for long-name '%s' - FOUND - returning the shortname '%s'", longName, contextItemP->name));

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
