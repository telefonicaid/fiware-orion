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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "orionld/kjTree/kjJsonldNullObject.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// kjJsonldNullObject -
//
// { "@type": "@json", "@value": null }
//
bool kjJsonldNullObject(KjNode* attrP, KjNode* typeP)
{
  char* type = typeP->name;

  if (*type == '@')
    ++type;

  if ((strcmp(type, "type") == 0) && (typeP->type == KjString))
  {
    if (strcmp(typeP->value.s, "@json") == 0)
    {
      KjNode* atValueP = kjLookup(attrP, "@value");

      if ((atValueP != NULL) && (atValueP->type == KjNull))
      {
        //
        // All good so far - both items present
        // Now, is that all?
        //
        int items = 0;
        for (KjNode* itemP = attrP->value.firstChildP; itemP != NULL; itemP = itemP->next)
        {
          ++items;
        }

        if (items == 2)  // All good - it's a JSON-LD NULL object
        {
          attrP->type    = KjNull;
          attrP->value.i = 0;

          return true;
        }
      }
    }
  }

  return false;
}
