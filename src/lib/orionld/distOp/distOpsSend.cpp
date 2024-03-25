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
#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/distOp/distOpSend.h"                              // distOpSend
#include "orionld/distOp/xForwardedForCompose.h"                    // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                              // viaCompose
#include "orionld/distOp/distOpsSend.h"                             // Own interface



// -----------------------------------------------------------------------------
//
// distOpsSend -
//
int distOpsSend(DistOp* distOpList, bool local)
{
  char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
  char* via = viaCompose(orionldState.in.via, brokerId);

  char dateHeader[70];
  snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);  // MOVE to orionldStateInit, for example

  int forwards = 0;  // Debugging purposees
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    // Send the forwarded request and await all responses
    if ((distOpP->regP != NULL) && (distOpP->error == false))
    {
      distOpP->onlyIds = true;

      if (distOpSend(distOpP, dateHeader, xff, via, local, NULL) == 0)
        distOpP->error = false;
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
        return -1;
      }

      if (stillRunning != 0)
      {
        cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
        if (cm != CURLM_OK)
        {
          LM_E(("Internal Error (curl_multi_wait: error %d", cm));
          return -2;
        }
      }

      if ((++loops >= 50) && ((loops % 25) == 0))
        LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
    }

    if (loops >= 100)
      LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));
  }

  return forwards;
}



// -----------------------------------------------------------------------------
//
// distOpsSend2 -
//
int distOpsSend2(DistOpListItem* distOpList)
{
  char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
  char* via = viaCompose(orionldState.in.via, brokerId);

  char dateHeader[70];
  snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);  // MOVE to orionldStateInit, for example

  int forwards = 0;  // Debugging purposees
  for (DistOpListItem* doItemP = distOpList; doItemP != NULL; doItemP = doItemP->next)
  {
    DistOp* distOpP = doItemP->distOpP;

    // Send the forwarded request and await all responses
    if ((distOpP->regP != NULL) && (distOpP->error == false))
    {
      distOpP->onlyIds = false;

      if (distOpSend(distOpP, dateHeader, xff, via, false, doItemP->entityIds) == 0)
        distOpP->error = false;
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
        return -1;
      }

      if (stillRunning != 0)
      {
        cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 5000, NULL);
        if (cm != CURLM_OK)
        {
          LM_E(("Internal Error (curl_multi_wait: error %d", cm));
          return -2;
        }

        if (loops > 10000)
        {
          LM_E(("Internal Error (curl_multi_wait: timeout (%d loops)", loops));
          return -3;
        }
      }

      if ((++loops >= 1000) && ((loops % 100) == 0))
        LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
    }

    if (loops >= 1000)
      LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));
  }

  return forwards;
}
