/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <unistd.h>                                            // NULL
#include <string.h>                                            // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove
}

#include "orionld/kjTree/kjSysAttrsRemove.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// kjSysAttrsRemove -
//
// PARAMETERS
//   container      - the toplevel KjNode from where to remove the sysAttrs (createdAt+modifiedAt)
//   recursionLevel - while it is > 0, we go down one level (0 => single level, no recursion)
//
void kjSysAttrsRemove(KjNode* container, int recursionLevel)
{
  KjNode* createdAtP  = kjLookup(container, "createdAt");
  KjNode* modifiedAtP = kjLookup(container, "modifiedAt");

  if (createdAtP != NULL)
    kjChildRemove(container, createdAtP);

  if (modifiedAtP != NULL)
    kjChildRemove(container, modifiedAtP);

  if (recursionLevel > 0)
  {
    for (KjNode* childP = container->value.firstChildP; childP != NULL; childP = childP->next)
    {
      if ((childP->type == KjObject) && (strcmp(childP->name, "value") != 0) && (strcmp(childP->name, "languageMap") != 0))
        kjSysAttrsRemove(childP, recursionLevel - 1);
    }
  }
}
