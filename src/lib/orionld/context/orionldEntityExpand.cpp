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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/context/orionldEntityExpand.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// orionldEntityExpand -
//
void orionldEntityExpand(KjNode* entityP, OrionldContext* contextP)
{
  KjNode* typeP = kjLookup(entityP, "type");

  if (typeP != NULL)
    typeP->value.s = orionldContextItemExpand(contextP, typeP->value.s, true, NULL);

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    // All attributes RHS are objects  (or Array once datasetId gets implemented)
    if (attrP->type == KjObject)
    {
      attrP->name = orionldAttributeExpand(contextP, attrP->name, true, NULL);

      for (KjNode* saP = attrP->value.firstChildP; saP != NULL; saP = saP->next)
      {
        saP->name = orionldSubAttributeExpand(contextP, saP->name, true, NULL);
      }
    }
  }
}
