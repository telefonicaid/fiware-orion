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
#include <curl/curl.h>                                              // curl_multi_info_read, curl_easy_getinfo, ...

extern "C"
{
#include "kjson/kjParse.h"                                          // kjParse
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/types/DistOpListItem.h"                           // DistOpListItem
#include "orionld/common/orionldState.h"                            // orionldState, entityMaps
#include "orionld/distOp/distOpLookupByCurlHandle.h"                // distOpLookupByCurlHandle
#include "orionld/distOp/distOpResponseMergeIntoEntityArray.h"      // distOpResponseMergeIntoEntityArray
#include "orionld/distOp/distOpsReceive2.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// distOpsReceive2 - FIXME: try to combine with distOpsReceive()
//
void distOpsReceive2(DistOpListItem* distOpListItem, DistOpResponseTreatFunction treatFunction, void* callbackParam)
{
  LM_T(LmtSR, ("Receiving responses"));
  //
  // Read the responses to the forwarded requests
  //
  CURLMsg* msgP;
  int      msgsLeft;

  while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
  {
    if (msgP->msg != CURLMSG_DONE)
      continue;

    if (msgP->data.result == CURLE_OK)
    {
      DistOp* distOpP = distOpLookupByCurlHandle(orionldState.distOpList, msgP->easy_handle);

      if (distOpP == NULL)
      {
        LM_E(("Unable to find the curl handle of a message, presumably a response to a forwarded request"));
        continue;
      }

      curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &distOpP->httpResponseCode);

      if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
        distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);

      LM_T(LmtDistOpResponse, ("%s: received a response for a forwarded request", distOpP->regP->regId, distOpP->httpResponseCode));
      LM_T(LmtDistOpResponse, ("%s: response for a forwarded request: %s", distOpP->regP->regId, distOpP->rawResponse));

      //
      // Treating here all non-auxiliar registrations.
      // The Auxiliar registrations are treated LAST
      //
      if (distOpP->regP->mode != RegModeAuxiliary)
        treatFunction(distOpP, callbackParam);
    }
  }

  //
  // Now that all non-auxiliar registrations have been treated, time for the aux regs
  //
  for (DistOp* distOpP = orionldState.distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    if ((distOpP->regP != NULL) && (distOpP->regP->mode == RegModeAuxiliary))
      distOpResponseMergeIntoEntityArray(distOpP, (KjNode*) callbackParam);
  }
}
