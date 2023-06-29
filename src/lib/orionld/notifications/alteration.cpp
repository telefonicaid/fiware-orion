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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration
#include "orionld/notifications/alteration.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// alteration -
//
OrionldAlteration* alteration(const char* entityId, const char* entityType, KjNode* apiEntityP, KjNode* incomingP, KjNode* dbEntityBeforeP)
{
  OrionldAlteration* alterationP = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));

  if (entityType == NULL)
  {
    KjNode* typeP = kjLookup(apiEntityP, "type");

    if (typeP != NULL)
      entityType = typeP->value.s;
  }

  alterationP->entityId          = (char*) entityId;
  alterationP->entityType        = (char*) entityType;
  alterationP->inEntityP         = incomingP;
  alterationP->dbEntityP         = dbEntityBeforeP;
  alterationP->finalApiEntityP   = apiEntityP;
  alterationP->alteredAttributes = 0;
  alterationP->alteredAttributeV = NULL;
  alterationP->next              = NULL;

  // Link it into the end of the alteration list
  if (orionldState.alterationsTail != NULL)
    orionldState.alterationsTail->next = alterationP;
  else
    orionldState.alterations = alterationP;

  orionldState.alterationsTail = alterationP;

  LM_T(LmtAlt, ("Added alteration for entity '%s'", entityId));

  return alterationP;
}
