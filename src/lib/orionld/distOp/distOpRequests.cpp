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
#include <stdio.h>                                               // snprintf

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpSend.h"                           // distOpSend
#include "orionld/distOp/xForwardedForCompose.h"                 // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                           // viaCompose
#include "orionld/regMatch/regMatchForEntityCreation.h"          // regMatchForEntityCreation



// -----------------------------------------------------------------------------
//
// purgeRedirectedAttributes -
//
// After the redirected registrations are dealt with, we need to chop attributes off the request tree (distOpP->requestBody).
// And after that continue with Inclusive registrations (and local DB)
//
static void purgeRedirectedAttributes(DistOp* redirectList, KjNode* body)
{
  for (DistOp* distOpP = redirectList; distOpP != NULL; distOpP = distOpP->next)
  {
    for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      KjNode* bodyAttrP = kjLookup(body, attrP->name);

      if (bodyAttrP != NULL)
        kjChildRemove(body, bodyAttrP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// distOpRequests -
//
// NOTE:  Need to be very careful here that we treat first all EXCLUSIVE registrations, as they "chop off" attributes to be forwarded
//        The other two (redirect, inclusive) don't, BUT only the remaining attributes AFTER the Exclusive regs have chopped off
//        attributes shall be forwarded for redirect+inclusive registrations.
//        Auxiliary registrations aren't allowed to receive create/update forwardes messages
//
// The fix is as follows:
//   DistOp* exclusiveList = regMatchForEntityCreation(RegModeExclusive, DoCreateEntity, entityId, entityType, orionldState.requestTree); (chopping attrs off)
//   DistOp* redirectList  = regMatchForEntityCreation(RegModeRedirect,  DoCreateEntity, entityId, entityType, orionldState.requestTree);
//
//   purgeRedirectedAttributes(redirectList, orionldState.requestTree);
//
//   DistOp* inclusiveList = regMatchForEntityCreation(RegModeInclusive, DoCreateEntity, entityId, entityType, orionldState.requestTree);
//
//   distOpList = distOpListsMerge(exclusiveList, redirectList);
//   distOpList = distOpListsMerge(distOpList, inclusiveList);
//
//   - Loop over distOpList
//
// Example:
// - The Entity to be created (and distributed) has "id" "urn:E1", "type": "T" and 10 Properties P1-P10
// - Exclusive registration R1 of urn:E1/T+P1
// - Exclusive registration R2 of urn:E1/T+P2
// - Redirect  registration R3 of urn:E1/T+P3+P4+P5 (or just T+P3+P4+P5)
// - Redirect  registration R4 of urn:E1/T+P4+P5+P6
// - Inclusive registration R5 of urn:E1/T+P7+P8+P9+P10 (not possible to include P1-P6)
//
// The entity will be distributed like this:
// - P1 on R1::endpoint
// - P2 on R2::endpoint
// - P3-P5 on R3::endpoint
// - P4-P6 on R4::endpoint
// - P7-P10 on R5::endpoint
// - P7-P10 on local broker
//
DistOp* distOpRequests(char* entityId, char* entityType, DistOpType operation, KjNode* payloadBody)
{
  char dateHeader[70];
  snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);  // MOVE to orionldStateInit, for example

  DistOp* distOpList    = NULL;
  DistOp* exclusiveList = NULL;
  DistOp* redirectList  = NULL;
  DistOp* inclusiveList = NULL;

  exclusiveList = regMatchForEntityCreation(RegModeExclusive, operation, entityId, entityType, payloadBody);
  redirectList  = regMatchForEntityCreation(RegModeRedirect,  operation, entityId, entityType, payloadBody);

  if (redirectList != NULL)
    purgeRedirectedAttributes(redirectList, payloadBody);  // chopping attrs off payloadBody

  inclusiveList = regMatchForEntityCreation(RegModeInclusive, operation, entityId, entityType, payloadBody);

  distOpList = distOpListsMerge(exclusiveList, redirectList);
  distOpList = distOpListsMerge(distOpList, inclusiveList);

  if (distOpList == NULL)
    return NULL;

  // Now that we've found all matching registrations we can add ourselves to the X-forwarded-For header
  char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
  char* via = viaCompose(orionldState.in.via, brokerId);

  // Enqueue all forwarded requests
  int forwards = 0;  // Debugging purposees
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    // Send the forwarded request and await all responses
    if ((distOpP->regP != NULL) && (distOpP->error == false))
    {
      if (distOpSend(distOpP, dateHeader, xff, via, false, NULL) == 0)
      {
        distOpP->error = false;
        orionldState.distOp.requests += 1;
     }
      else
        distOpP->error = true;

      ++forwards;  // Also when error?
    }
  }

  int stillRunning = 1;
  int loops        = 0;

  if (forwards > 0)
  {
    while (stillRunning != 0)
    {
      CURLMcode cm = curl_multi_perform(orionldState.curlDoMultiP, &stillRunning);
      if (cm != 0)
      {
        LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
        forwards = 0;
        break;
      }

      if (stillRunning != 0)
      {
        cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
        if (cm != CURLM_OK)
        {
          LM_E(("Internal Error (curl_multi_wait: error %d", cm));
          break;
        }
      }

      if ((++loops >= 50) && ((loops % 25) == 0))
        LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
    }

    if (loops >= 100)
      LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));

    // Anything left for a local entity?
    if (payloadBody->value.firstChildP != NULL)
    {
      if (operation == DoCreateEntity)
      {
        KjNode* entityIdP   = kjString(orionldState.kjsonP,  "id",   entityId);
        KjNode* entityTypeP = kjString(orionldState.kjsonP,  "type", entityType);

        kjChildAdd(payloadBody, entityIdP);
        kjChildAdd(payloadBody, entityTypeP);
      }
    }
    else
      orionldState.requestTree = NULL;  // Meaning: nothing left for local DB
  }

  return distOpList;
}
