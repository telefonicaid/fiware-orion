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

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/RegistrationMode.h"                      // registrationMode
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/xForwardedForMatch.h"                   // xForwardedForMatch
#include "orionld/distOp/viaMatch.h"                             // viaMatch
#include "orionld/regMatch/regMatchOperation.h"                  // regMatchOperation
#include "orionld/regMatch/regMatchInformationArrayForGet.h"     // regMatchInformationArrayForGet
#include "orionld/regMatch/regMatchForEntityGet.h"               // Own interface



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
DistOp* regMatchForEntityGet  // FIXME: +entity-type
(
  RegistrationMode regMode,
  DistOpType       operation,
  const char*      entityId,
  const char*      entityType,
  StringArray*     attrV,
  const char*      geoProp
)
{
  DistOp* distOpHead = NULL;
  DistOp* distOpTail = NULL;

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    if (regP->regId == NULL)
    {
      KjNode* regIdP = kjLookup(regP->regTree, "id");
      regP->regId = (regIdP != NULL)? regIdP->value.s : (char*) "unknown registration";
    }

    if ((regP->mode & regMode) == 0)
    {
       LM_T(LmtRegMatch, ("%s: No match due to regMode", regP->regId));
       continue;
    }

    // Loop detection
    if (viaMatch(orionldState.in.via, regP->hostAlias) == true)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Loop (Via)", regP->regId));
      continue;
    }

    if (xForwardedForMatch(orionldState.in.xForwardedFor, regP->ipAndPort) == true)
    {
      LM_T(LmtRegMatch, ("No Reg Match due to loop detection"));
      continue;
    }

    if ((regMode != RegModeExclusive) && (regMatchOperation(regP, operation) == false))
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Operation", regP->regId));
      continue;
    }

    DistOp* distOpP = regMatchInformationArrayForGet(regP, entityId, entityType, attrV, geoProp);
    if (distOpP == NULL)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Information Array", regP->regId));
      continue;
    }

    if ((regMode == RegModeExclusive) && (regMatchOperation(regP, operation) == false))
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Operation (operation == %d: '%s')", regP->regId, operation, distOpTypes[operation]));
      for (DistOp* doP = distOpP; doP != NULL; doP = doP->next)
      {
        doP->error            = true;
        doP->title            = (char*) "Operation not supported";
        doP->detail           = (char*) "A matching exclusive registration forbids the Operation";
        doP->httpResponseCode = 409;
      }
      continue;
    }

    // Add extra info in DistOp, needed by forwardRequestSend
    distOpP->entityId   = (char*) entityId;
    distOpP->entityType = (char*) entityType;
    distOpP->operation  = operation;

    // Add distOpP to the linked list
    if (distOpHead == NULL)
      distOpHead = distOpP;
    else
      distOpTail->next = distOpP;

    distOpTail       = distOpP;
    distOpTail->next = NULL;

    LM_T(LmtRegMatch, ("%s: Reg Match !", regP->regId));
  }

  return distOpHead;
}
