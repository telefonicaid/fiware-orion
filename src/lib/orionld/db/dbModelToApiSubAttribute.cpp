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
extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/db/dbModelToApiSubAttribute.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiSubAttribute -
//
void dbModelToApiSubAttribute(KjNode* subP)
{
  //
  // Remove unwanted parts of the sub-attribute from DB
  //
  const char* unwanted[] = { "createdAt", "modifiedAt" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(subP, unwanted[ix]);

    if (nodeP != NULL)
      kjChildRemove(subP, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// dbModelToApiSubAttribute2 - transform a sub-attribute from DB Model to API format
//
KjNode* dbModelToApiSubAttribute2(KjNode* dbSubAttributeP, bool sysAttrs, OrionldProblemDetails* pdP)
{
  char*   longName = kaStrdup(&orionldState.kalloc, dbSubAttributeP->name);
  eqForDot(longName);

  char*   alias    = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* mdP      = kjObject(orionldState.kjsonP, alias);
  KjNode* nodeP    = dbSubAttributeP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;
    if      (strcmp(nodeP->name, "type")       == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "value")      == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "object")     == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "unitCode")   == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "observedAt") == 0) kjChildAdd(mdP, nodeP);
    else if (sysAttrs == true)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        nodeP->name = (char*) "createdAt";
        kjChildAdd(mdP, nodeP);
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        nodeP->name = (char*) "modifiedAt";
        kjChildAdd(mdP, nodeP);
      }
    }
    else
      LM_W(("Skipping sub-sub-attribute '%s'", nodeP->name));

    nodeP = next;
  }

  return mdP;
}



