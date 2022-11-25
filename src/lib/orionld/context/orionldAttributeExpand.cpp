/*
*
* Copyright 2021 FIWARE Foundation e.V.
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

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/orionldContextItemAlreadyExpanded.h"   // orionldContextItemAlreadyExpanded
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // Own interface



// -----------------------------------------------------------------------------
//
// orionldAttributeExpand -
//
// This function expands unless:
//   - has already been expanded
//   - is a special attribute such as 'location'
//
char* orionldAttributeExpand
(
  OrionldContext*       contextP,
  char*                 shortName,
  bool                  useDefaultUrlIfNotFound,
  OrionldContextItem**  contextItemPP
)
{
  if      (strcmp(shortName, "id")               == 0) return shortName;
  else if (strcmp(shortName, "@id")              == 0) return shortName;
  else if (strcmp(shortName, "type")             == 0) return shortName;
  else if (strcmp(shortName, "@type")            == 0) return shortName;
  else if (strcmp(shortName, "scope")            == 0) return shortName;
  else if (strcmp(shortName, "location")         == 0) return shortName;
  else if (strcmp(shortName, "createdAt")        == 0) return shortName;
  else if (strcmp(shortName, "modifiedAt")       == 0) return shortName;
  else if (strcmp(shortName, "observationSpace") == 0) return shortName;
  else if (strcmp(shortName, "operationSpace")   == 0) return shortName;

#if 1
  // FIXME: 'observedAt' as an attribute is not a thing - special treatment only if sub-attribute
  //  The implementation needs modifications.
  //
  //  Wait ... what if the @context says it must be a String, a DateTime?
  //  The @context doesn't distinguish between attributesd and sub-attributes ...
  //  It will need to be a String wherever it is.
  //
  //  We should probably forbid an attribute to have the name 'observedAt'
  //
  else if (strcmp(shortName, "observedAt") == 0)
  {
    orionldContextItemExpand(contextP, shortName, false, contextItemPP);
    return shortName;
  }
#endif

  if (orionldContextItemAlreadyExpanded(shortName) == true)
    return shortName;

  return orionldContextItemExpand(contextP, shortName, useDefaultUrlIfNotFound, contextItemPP);
}
