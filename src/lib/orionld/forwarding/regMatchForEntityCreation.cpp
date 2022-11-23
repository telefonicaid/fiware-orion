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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/RegistrationMode.h"                      // registrationMode
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/FwdOperation.h"                     // FwdOperation
#include "orionld/forwarding/regMatchOperation.h"                // regMatchOperation
#include "orionld/forwarding/regMatchInformationArray.h"         // regMatchInformationArray
#include "orionld/forwarding/regMatchForEntityCreation.h"        // Own interface



// -----------------------------------------------------------------------------
//
// regMatchForEntityCreation -
//
// To match for Entity Creation, a registration needs:
// - "mode" != "auxiliary"
// - "operations" must include "createEntity
// - "information" must match by entity id+type and attributes if present in the registration
//
ForwardPending* regMatchForEntityCreation
(
  RegistrationMode regMode,
  FwdOperation     operation,
  const char*      entityId,
  const char*      entityType,
  KjNode*          incomingP
)
{
  ForwardPending* fwdPendingHead = NULL;
  ForwardPending* fwdPendingTail = NULL;

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    if ((regP->mode & regMode) == 0)
    {
      LM(("No Reg Match due to regMode"));
      continue;
    }

    if (regMatchOperation(regP, operation) == false)
    {
      LM(("No Reg Match due to Operation"));
      continue;
    }

    ForwardPending* fwdPendingP = regMatchInformationArray(regP, entityId, entityType, incomingP);
    if (fwdPendingP == NULL)
    {
      LM(("No Reg Match due to Information Array"));
      continue;
    }

    // Add extra info in ForwardPending, needed by forwardRequestSend
    fwdPendingP->operation = operation;

    // Add fwdPendingP to the linked list
    if (fwdPendingHead == NULL)
      fwdPendingHead = fwdPendingP;
    else
      fwdPendingTail->next = fwdPendingP;

    fwdPendingTail       = fwdPendingP;
    fwdPendingTail->next = NULL;
  }

  return fwdPendingHead;
}
