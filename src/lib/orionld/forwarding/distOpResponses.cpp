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
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/curlToBrokerStrerror.h"                 // curlToBrokerStrerror
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/distOpLookupByCurlHandle.h"         // distOpLookupByCurlHandle
#include "orionld/forwarding/distOpSuccess.h"                    // distOpSuccess
#include "orionld/forwarding/distOpFailure.h"                    // distOpFailure



// -----------------------------------------------------------------------------
//
// distOpResponses -
//
void distOpResponses(DistOp* distOpList, KjNode* responseBody)
{
  CURLMsg* msgP;
  int      msgsLeft;

  while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
  {
    if (msgP->msg != CURLMSG_DONE)
      continue;

    DistOp* distOpP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);

    int httpResponseCode = 500;
    curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
    LM_T(LmtDistOpMsgs, ("Reg %s: HTTP Response Code: %d", distOpP->regP->regId, httpResponseCode));

    if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
      LM_T(LmtDistOpMsgs, ("Response Body: '%s'", distOpP->rawResponse));

    if (msgP->data.result == CURLE_OK)
      distOpSuccess(responseBody, distOpP);
    else
    {
      int          statusCode = 500;
      const char*  detail     = curlToBrokerStrerror(msgP->easy_handle, msgP->data.result, &statusCode);

      LM_E(("CURL Error %d: %s", msgP->data.result, curl_easy_strerror(msgP->data.result)));
      distOpFailure(responseBody, distOpP, OrionldInternalError, "Error during Distributed Operation", detail, statusCode);
    }
  }
}
