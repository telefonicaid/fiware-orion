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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/kjTree/kjSysAttrsRemove.h"                     // kjSysAttrsRemove
#include "orionld/apiModel/ntonAttribute.h"                      // ntonAttribute
#include "orionld/apiModel/ntonEntity.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// ntonEntity -
//
void ntonEntity(KjNode* apiEntityP, const char* lang, bool sysAttrs)
{
  if (sysAttrs == false)
    kjSysAttrsRemove(apiEntityP, 0);

  for (KjNode* fieldP = apiEntityP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    if (strcmp(fieldP->name, "id")          == 0)  continue;
    if (strcmp(fieldP->name, "type")        == 0)  continue;

    if (sysAttrs == true)
    {
      if (strcmp(fieldP->name, "createdAt")   == 0)  continue;
      if (strcmp(fieldP->name, "modifiedAt")  == 0)  continue;
    }

    // It's an attribute
    ntonAttribute(fieldP, lang, sysAttrs);
  }
}
