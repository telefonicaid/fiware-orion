/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/RegistrationMode.h"                      // RegistrationMode
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/regMatch/regMatchOperation.h"                  // regMatchOperation
#include "orionld/regMatch/regMatchInformationArrayForQuery.h"   // regMatchInformationArrayForQuery
#include "orionld/distOp/viaMatch.h"                             // viaMatch
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/regMatch/regMatchForEntitiesQuery.h"           // Own interface



// -----------------------------------------------------------------------------
//
// regMatchForEntitiesquery -
//
DistOp* regMatchForEntitiesQuery
(
  RegistrationMode  regMode,
  DistOpType        opType,
  StringArray*      idListP,
  StringArray*      typeListP,
  StringArray*      attrListP
)
{
  DistOp* distOpList = NULL;

  LM_W(("Looping over regCache of tenant '%s' (cache at %p, first reg at %p)", orionldState.tenantP->mongoDbName, orionldState.tenantP->regCache, orionldState.tenantP->regCache->regList));
  for (RegCacheItem* regP = orionldState.tenantP->regCache->regList; regP != NULL; regP = regP->next)
  {
    LM_W(("In Loop"));
    if ((regP->mode & regMode) == 0)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to RegistrationMode ('%s' vs '%s')", regP->regId, registrationModeToString(regP->mode), registrationModeToString(regMode)));
      continue;
    }

    if (regMatchOperation(regP, opType) == false)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Operation (operation == '%s')", regP->regId, distOpTypeToString(opType)));
      continue;
    }

    // Loop detection
    if (viaMatch(orionldState.in.via, regP->hostAlias) == true)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Loop (Via)", regP->regId));
      continue;
    }

    DistOp* distOpP = regMatchInformationArrayForQuery(regP, idListP, typeListP, attrListP);
    if (distOpP == NULL)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to Information Array", regP->regId));
      continue;
    }

    //
    // Add distOpP to the linked list (distOpList)
    //
    LM_T(LmtRegMatch, ("%s: Reg Match !", regP->regId));

    distOpList = distOpListsMerge(distOpList, distOpP);
  }

  return distOpList;
}
