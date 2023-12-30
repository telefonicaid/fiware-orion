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

#include "logMsg/logMsg.h"                                       // LM_T

#include "orionld/types/RegistrationMode.h"                      // registrationMode
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/xForwardedForMatch.h"                   // xForwardedForMatch
#include "orionld/distOp/viaMatch.h"                             // viaMatch
#include "orionld/regMatch/regMatchOperation.h"                  // regMatchOperation
#include "orionld/regMatch/regMatchInformationArray.h"           // regMatchInformationArray
#include "orionld/regMatch/regMatchForEntityCreation.h"          // Own interface


#if 0
// -----------------------------------------------------------------------------
//
// distOpListDebug -
//
void distOpListDebug(DistOp* distOpP, const char* what)
{
  LM_T(LmtDistOpList, ("----- DistOp List: %s", what));

  while (distOpP != NULL)
  {
    LM_T(LmtDistOpList, ("  Registration:      %s", distOpP->regP->regId));
    LM_T(LmtDistOpList, ("  Operation:         %s", distOpTypes[distOpP->operation]));

    if (distOpP->error == true)
    {
      LM_T(LmtDistOpList, ("  Title:             %s", distOpP->title));
      LM_T(LmtDistOpList, ("  Detail:            %s", distOpP->detail));
      LM_T(LmtDistOpList, ("  Status:            %d", distOpP->httpResponseCode));
    }

    if (distOpP->requestBody != NULL)
    {
      LM_T(LmtDistOpList, ("  Attributes:"));
      int ix = 0;
      for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        if ((strcmp(attrP->name, "id") != 0) && (strcmp(attrP->name, "type") != 0))
        {
          LM_T(LmtDistOpList, ("    Attribute %d:   '%s'", ix, attrP->name));
          ++ix;
        }
      }
    }

    if (distOpP->attrList != NULL)
    {
      LM_T(LmtDistOpList, ("  URL Attributes:        %d", distOpP->attrList->items));
      for (int ix = 0; ix < distOpP->attrList->items; ix++)
      {
        LM_T(LmtDistOpList, ("    Attribute %d:   '%s'", ix, distOpP->attrList->array[ix]));
      }
    }

    distOpP = distOpP->next;
  }

  LM_T(LmtDistOpList, ("---------------------"));
}
#endif



// -----------------------------------------------------------------------------
//
// regMatchForEntityCreation -
//
// To match for Entity Creation, a registration needs:
// - "mode" != "auxiliary"
// - "operations" must include "createEntity
// - "information" must match by entity id+type and attributes if present in the registration
//
DistOp* regMatchForEntityCreation
(
  RegistrationMode regMode,     // Exclusive, Redirect, Inclusive, Auxiliar
  DistOpType       operation,   // createEntity, patchAttribute, ...
  const char*      entityId,
  const char*      entityType,
  KjNode*          payloadBody
)
{
  DistOp* distOpHead = NULL;
  DistOp* distOpTail = NULL;

  LM_T(LmtRegMatch, ("Registration Mode: %d (%s)", regMode, registrationModeToString(regMode)));
  LM_T(LmtRegMatch, ("Operation:         %d (%s)", operation, distOpTypes[operation]));

  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    if ((regP->mode & regMode) == 0)
    {
      // LM_T(LmtRegMatch, ("%s: No Reg Match due to regMode (0x%x vs 0x%x)", regP->regId, regP->mode, regMode));
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
      LM_T(LmtRegMatch, ("%s: No Reg Match due to loop detection", regP->regId));
      continue;
    }

    if ((regMode != RegModeExclusive) && (regMatchOperation(regP, operation) == false))
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Operation (operation == %d: '%s')", regP->regId, operation, distOpTypes[operation]));
      continue;
    }

    DistOp* distOpP = regMatchInformationArray(regP, operation, entityId, entityType, payloadBody);
    if (distOpP == NULL)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Information Array", regP->regId));
      continue;
    }

    LM_T(LmtRegMatch, ("%s: Match!", regP->regId));

    //
    // If Exclusive, we now need to check the Operation (DistOpType)
    // If not a match, the distOpP needs to be marked as ERROR (409)
    //
    if ((regMode == RegModeExclusive) && (regMatchOperation(regP, operation) == false))
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to 'matching exclusive registration forbids the Operation' (operation == %d: '%s')", regP->regId, operation, distOpTypes[operation]));
      for (DistOp* doP = distOpP; doP != NULL; doP = doP->next)
      {
        doP->error            = true;
        doP->title            = (char*) "Operation not supported";
        doP->detail           = (char*) "A matching exclusive registration forbids the Operation";
        doP->httpResponseCode = 409;
      }
    }

    // Add extra info in DistOp, needed by forwardRequestSend
    distOpP->operation = operation;

    // Add distOpP to the linked list
    if (distOpHead == NULL)
      distOpHead = distOpP;
    else
      distOpTail->next = distOpP;

    distOpTail       = distOpP;
    distOpTail->next = NULL;
  }

  return distOpHead;
}
