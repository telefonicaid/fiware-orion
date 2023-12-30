/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjBuilder.h"                                        // kjChildRemove, kjChildAdd, ...
#include "kjson/kjLookup.h"                                         // kjLookup
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/common/orionldState.h"                            // orionldState, entityMaps
#include "orionld/kjTree/kjEntityIdLookupInEntityArray.h"           // kjEntityIdLookupInEntityArray
#include "orionld/distOp/distOpEntityMerge.h"                       // distOpEntityMerge
#include "orionld/distOp/distOpResponseMergeIntoEntityArray.h"      // Own interface



// -----------------------------------------------------------------------------
//
// distOpResponseMergeIntoEntityArray -
//
void distOpResponseMergeIntoEntityArray(DistOp* distOpP, KjNode* entityArray)
{
  LM_W(("Merging entities for DistOp '%s' (aux: %s)", distOpP->id, (distOpP->regP->mode == RegModeAuxiliary)? "YES" : "NO"));
  LM_T(LmtSR, ("Got a response. status code: %d. entityArray: %p", distOpP->httpResponseCode, entityArray));

  kjTreeLog(distOpP->responseBody, "Response", LmtSR);

  if ((distOpP->httpResponseCode == 200) && (distOpP->responseBody != NULL))
  {
    LM_T(LmtSR, ("Got a body from endpoint registered in reg '%s'", distOpP->regP->regId));
    LM_T(LmtSR, ("Must merge these new entities with the ones already received"));

    KjNode* entityP = distOpP->responseBody->value.firstChildP;
    KjNode* next;
    while (entityP != NULL)
    {
      next = entityP->next;
      kjChildRemove(distOpP->responseBody, entityP);

      // Lookup the entity in entityArray and:
      // - Add entityP if not found in entityArray
      // - Merge the two if found in entityArray
      //
      KjNode* entityIdNodeP = kjLookup(entityP, "id");

      if (entityIdNodeP != NULL)
      {
        char*   entityId    = entityIdNodeP->value.s;
        KjNode* baseEntityP = kjEntityIdLookupInEntityArray(entityArray, entityId);

        if (baseEntityP == NULL)
        {
          LM_T(LmtDistOpMerge, ("New Entity '%s' - adding it to the entity array (at %p)", entityId, entityArray));
          kjChildAdd(entityArray, entityP);
        }
        else
        {
          LM_T(LmtDistOpMerge, ("Existing Entity '%s' - merging it in the entity array (reg-mode: %s)", entityId, registrationModeToString(distOpP->regP->mode)));
          distOpEntityMerge(baseEntityP, entityP, orionldState.uriParamOptions.sysAttrs, distOpP->regP->mode == RegModeAuxiliary);
        }
      }
      else
        LM_W(("No Entity ID in response from forwarded GET /entities request (reg %s)", distOpP->regP->regId));

      entityP = next;
    }
  }
}

