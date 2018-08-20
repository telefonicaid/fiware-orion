/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "logMsg/logMsg.h"

#include "common/MimeType.h"
#include "common/limits.h"
#include "ngsi/StatusCode.h"
#include "metricsMgr/metricsMgr.h"

#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"
#include "ngsi9/NotifyContextAvailabilityResponse.h"

#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "ngsi10/UnsubscribeContextResponse.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"
#include "ngsi10/NotifyContextResponse.h"

#include "rest/rest.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/HttpStatusCode.h"
#include "rest/HttpHeaders.h"
#include "rest/mhd.h"
#include "rest/OrionError.h"
#include "rest/restReply.h"

#include "logMsg/traceLevels.h"



/* ****************************************************************************
*
* replyIx - counter of the replies
*/
static int replyIx = 0;



/* ****************************************************************************
*
* restReply -
*/
void restReply(ConnectionInfo* ciP, const std::string& answer)
{
  MHD_Response*  response;

  uint64_t       answerLen = answer.length();
  std::string    spath     = (ciP->servicePathV.size() > 0)? ciP->servicePathV[0] : "";

  ++replyIx;
  LM_T(LmtServiceOutPayload, ("Response %d: responding with %d bytes, Status Code %d", replyIx, answerLen, ciP->httpStatusCode));
  LM_T(LmtServiceOutPayload, ("Response payload: '%s'", answer.c_str()));

  response = MHD_create_response_from_buffer(answerLen, (void*) answer.c_str(), MHD_RESPMEM_MUST_COPY);
  if (!response)
  {
    metricsMgr.add(ciP->httpHeaders.tenant, spath, METRIC_TRANS_IN_ERRORS, 1);
    LM_E(("Runtime Error (MHD_create_response_from_buffer FAILED)"));

#ifdef ORIONLD
    if (ciP->responsePayloadAllocated == true)
      free(ciP->responsePayload);
#endif    

    return;
  }

  if (answerLen > 0)
  {
    metricsMgr.add(ciP->httpHeaders.tenant, spath, METRIC_TRANS_IN_RESP_SIZE, answerLen);
  }

  for (unsigned int hIx = 0; hIx < ciP->httpHeader.size(); ++hIx)
  {
    MHD_add_response_header(response, ciP->httpHeader[hIx].c_str(), ciP->httpHeaderValue[hIx].c_str());
  }

  if (answer != "")
  {
    if (ciP->outMimeType == JSON)
    {
      MHD_add_response_header(response, HTTP_CONTENT_TYPE, "application/json");
    }
    else if (ciP->outMimeType == TEXT)
    {
      MHD_add_response_header(response, HTTP_CONTENT_TYPE, "text/plain");
    }
  }

  // Check if CORS is enabled, the Origin header is present in the request and the response is not a bad verb response
  if ((corsEnabled == true) && (ciP->httpHeaders.origin != "") && (ciP->httpStatusCode != SccBadVerb))
  {
    // Only GET method is supported for V1 API
    if ((ciP->apiVersion == V2) || (ciP->apiVersion == V1 && ciP->verb == GET))
    {
      bool originAllowed = true;

      // If any origin is allowed, the header is sent always with "any" as value
      if (strcmp(corsOrigin, "__ALL") == 0)
      {
        MHD_add_response_header(response, HTTP_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
      }
      // If a specific origin is allowed, the header is only sent if the origins match
      else if (strcmp(ciP->httpHeaders.origin.c_str(), corsOrigin) == 0)
      {
        MHD_add_response_header(response, HTTP_ACCESS_CONTROL_ALLOW_ORIGIN, corsOrigin);
      }
      // If there is no match, originAllowed flag is set to false
      else
      {
        originAllowed = false;
      }

      // If the origin is not allowed, no headers are added to the response
      if (originAllowed)
      {
        // Add Access-Control-Expose-Headers to the response
        MHD_add_response_header(response, HTTP_ACCESS_CONTROL_EXPOSE_HEADERS, CORS_EXPOSED_HEADERS);

        if (ciP->verb == OPTIONS)
        {
          MHD_add_response_header(response, HTTP_ACCESS_CONTROL_ALLOW_HEADERS, CORS_ALLOWED_HEADERS);

          char maxAge[STRING_SIZE_FOR_INT];
          snprintf(maxAge, sizeof(maxAge), "%d", corsMaxAge);

          MHD_add_response_header(response, HTTP_ACCESS_CONTROL_MAX_AGE, maxAge);
        }
      }
    }
  }

  MHD_queue_response(ciP->connection, ciP->httpStatusCode, response);
  MHD_destroy_response(response);

#ifdef ORIONLD
    if (ciP->responsePayloadAllocated == true)
      free(ciP->responsePayload);
#endif    
}



/* ****************************************************************************
*
* restErrorReplyGet -
*
* This function renders an error reply depending on the type of the request (ciP->restServiceP->request).
*
* The function is called from more than one place, especially from
* restErrorReply(), but also from where the payload type is matched against the request URL.
* Where the payload type is matched against the request URL, the incoming 'request' is a
* request and not a response.
*/
void restErrorReplyGet(ConnectionInfo* ciP, HttpStatusCode code, const std::string& details, std::string* outStringP)
{
  StatusCode  errorCode(code, details, "errorCode");

  ciP->httpStatusCode = SccOk;

  if (ciP->restServiceP->request == RegisterContext)
  {
    RegisterContextResponse rcr("000000000000000000000000", errorCode);
    *outStringP = rcr.render();
  }
  else if (ciP->restServiceP->request == DiscoverContextAvailability)
  {
    DiscoverContextAvailabilityResponse dcar(errorCode);
    *outStringP = dcar.render();
  }
  else if (ciP->restServiceP->request == SubscribeContextAvailability)
  {
    SubscribeContextAvailabilityResponse scar("000000000000000000000000", errorCode);
    *outStringP = scar.render();
  }
  else if ((ciP->restServiceP->request == UpdateContextAvailabilitySubscription) || (ciP->restServiceP->request == Ngsi9SubscriptionsConvOp))
  {
    UpdateContextAvailabilitySubscriptionResponse ucas(errorCode);
    *outStringP = ucas.render();
  }
  else if (ciP->restServiceP->request == UnsubscribeContextAvailability)
  {
    UnsubscribeContextAvailabilityResponse ucar(errorCode);
    *outStringP = ucar.render();
  }
  else if (ciP->restServiceP->request == NotifyContextAvailability)
  {
    NotifyContextAvailabilityResponse ncar(errorCode);
    *outStringP = ncar.render();
  }
  else if (ciP->restServiceP->request == QueryContext)
  {
    QueryContextResponse  qcr(errorCode);
    bool                  asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
    *outStringP = qcr.render(ciP->apiVersion, asJsonObject);
  }
  else if (ciP->restServiceP->request == SubscribeContext)
  {
    SubscribeContextResponse scr(errorCode);
    *outStringP = scr.render();
  }
  else if ((ciP->restServiceP->request == UpdateContextSubscription) || (ciP->restServiceP->request == Ngsi10SubscriptionsConvOp))
  {
    UpdateContextSubscriptionResponse ucsr(errorCode);
    *outStringP = ucsr.render();
  }
  else if (ciP->restServiceP->request == UnsubscribeContext)
  {
    UnsubscribeContextResponse uncr(errorCode);
    *outStringP = uncr.render();
  }
  else if (ciP->restServiceP->request == UpdateContext)
  {
    UpdateContextResponse ucr(errorCode);
    bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
    *outStringP = ucr.render(ciP->apiVersion, asJsonObject);
  }
  else if (ciP->restServiceP->request == NotifyContext)
  {
    NotifyContextResponse ncr(errorCode);
    *outStringP = ncr.render();
  }
  else
  {
    OrionError oe(errorCode);

    LM_T(LmtRest, ("Unknown request type: '%d'", ciP->restServiceP->request));
    ciP->httpStatusCode = oe.code;
    *outStringP = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &ciP->httpStatusCode);
  }
}
