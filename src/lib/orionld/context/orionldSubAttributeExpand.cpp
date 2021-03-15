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
#include <string.h>                                              // strchr

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/orionldContextItemAlreadyExpanded.h"   // orionldContextItemAlreadyExpanded
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // Own interface


// -----------------------------------------------------------------------------
//
// orionldSubAttributeExpand -
//
// This function expands unless:
//   - has already been expanded
//   - is a special sub-attribute such as 'location', 'datasetyId', ...
//
char* orionldSubAttributeExpand
(
  OrionldContext*       contextP,
  char*                 shortName,
  bool                  useDefaultUrlIfNotFound,
  OrionldContextItem**  contextItemPP
)
{
  if (orionldContextItemAlreadyExpanded(shortName) == true)
    return shortName;

  if      (strcmp(shortName, "location")   == 0) return shortName;
  else if (strcmp(shortName, "observedAt") == 0) return shortName;
  else if (strcmp(shortName, "unitCode")   == 0) return shortName;
  else if (strcmp(shortName, "datasetId")  == 0) return shortName;

  return orionldContextItemExpand(contextP, shortName, useDefaultUrlIfNotFound, contextItemPP);
}
