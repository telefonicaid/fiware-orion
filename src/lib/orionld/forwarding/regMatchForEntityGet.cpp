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
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/regMatchOperation.h"                // regMatchOperation
#include "orionld/forwarding/regMatchInformationArrayForGet.h"   // regMatchInformationArrayForGet
#include "orionld/forwarding/regMatchForEntityGet.h"             // Own interface



// -----------------------------------------------------------------------------
//
// regMatchForEntityGet -
//
// To match for Entity Creation, a registration needs:
// - "operations" must include "retrieveEntity"
// - "information" must match by
//   - entity id and
//   - attributes if present in the registration (and 'attrs' URL param)
//
ForwardPending* regMatchForEntityGet
(
  RegistrationMode regMode,
  FwdOperation     operation,
  const char*      entityId,
  StringArray*     attrV,
  const char*      geoProp
)
{
  ForwardPending* fwdPendingHead = NULL;
  ForwardPending* fwdPendingTail = NULL;

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
#ifdef DEBUG
    KjNode* regIdP = kjLookup(regP->regTree, "id");
    char*   regId  = (char*) "unknown registration";
    if (regIdP != NULL)
      regId = regIdP->value.s;
    LM(("Treating registration '%s' for registrations of mode '%s'", regId, registrationModeToString(regMode)));
#endif

    if ((regP->mode & regMode) == 0)
    {
      LM(("%s: No Reg Match due to regMode", regId));
      continue;
    }

    if (regMatchOperation(regP, operation) == false)
    {
      LM(("%s: No Reg Match due to Operation", regId));
      continue;
    }
    ForwardPending* fwdPendingP = regMatchInformationArrayForGet(regP, entityId, attrV, geoProp);
    if (fwdPendingP == NULL)
    {
      LM(("%s: No Reg Match due to Information Array", regId));
      continue;
    }

    // Add extra info in ForwardPending, needed by forwardRequestSend
    fwdPendingP->entityId  = (char*) entityId;
    fwdPendingP->operation = operation;

    // Add fwdPendingP to the linked list
    if (fwdPendingHead == NULL)
      fwdPendingHead = fwdPendingP;
    else
      fwdPendingTail->next = fwdPendingP;

    fwdPendingTail       = fwdPendingP;
    fwdPendingTail->next = NULL;

    LM(("%s: Reg Match !", regId));
  }

  return fwdPendingHead;
}
