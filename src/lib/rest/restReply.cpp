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
#include "rest/mhd.h"
#include "rest/OrionError.h"
#include "rest/restReply.h"

#include "logMsg/traceLevels.h"

#ifdef PARANOID_JSON_INDENT
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "common/string.h"   // jsonFix
#endif

static int replyIx = 0;

/* ****************************************************************************
*
* restReply -
*/
void restReply(ConnectionInfo* ciP, const std::string& _answer)
{
  MHD_Response*  response;

  std::string answer;

#ifdef PARANOID_JSON_INDENT
  if (paranoidV1Indent)
  {
    // To mantain paranoid backward compatibility pretty print must be applied if
    //
    //  * Result is 415 Unsupported Media Type or 406 Not Acceptable, no matter the outMimeType, and API version is not v2
    //  * outMimeType is JSON and either
    //    * API version is v1, or
    //    * API version is no_version and status code is different from 200
    //
    // Apart from the above, answer must have effective payload (i.e. not empty)
    //
    bool prettyRenderedResponse = ((ciP->httpStatusCode == SccNotAcceptable) && (ciP->apiVersion != V2)) ||
        ((ciP->httpStatusCode == SccUnsupportedMediaType) && (ciP->apiVersion != V2)) ||
        ((ciP->outMimeType == JSON) && ((ciP->apiVersion == V1) ||
                                        ((ciP->apiVersion == NO_VERSION) && (ciP->httpStatusCode != SccOk))));

    // Exception: responses to ops on /statistics and /v1/admin/statistivcs URLs *must not* be pretty printted
    prettyRenderedResponse = prettyRenderedResponse && (!(ciP->url == "/statistics") || (ciP->url == "/v1/admin/statistics"));

    if (prettyRenderedResponse && (_answer.length() != 0))
    {
      // First stage: conventional pretty printer
      rapidjson::Document doc;
      doc.Parse(_answer.c_str());

      rapidjson::StringBuffer s;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
      writer.SetIndent(' ', 2);
      doc.Accept(writer);

      std::string prettyPrinted = s.GetString();

      // Second stage: "key": "value" -> "key" : value (legacy reasons... this was the
      // way we implement JSON rendering at the very beggining)
      char* prettyPrinted2 = jsonFix(prettyPrinted.c_str());
      answer = std::string(prettyPrinted2);
      free(prettyPrinted2);
    }
    else
    {
      answer = _answer;
    }
  }
  else  // paranoidV1Indent = false
  {
    answer = _answer;
  }
#else
  answer = _answer;
#endif

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
      MHD_add_response_header(response, "Content-Type", "application/json");
    }
    else if (ciP->outMimeType == TEXT)
    {
      MHD_add_response_header(response, "Content-Type", "text/plain");
    }
  }

  //Check if CORS is enabled
  if (strlen(restAllowedOrigin) > 0)
  {
    //Only GET method is supported for V1 API
    if (ciP->apiVersion == V2 || (ciP->apiVersion == V1 && ciP->verb == GET))
    {
      // If any origin is allowed, the header is sent always with "any" as value
      if (strcmp(restAllowedOrigin, "__ALL") == 0)
      {
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
      }
      // If a specific origin is allowed, the header is only sent if the origins match
      else if (strcmp(ciP->httpHeaders.origin.c_str(), restAllowedOrigin) == 0)
      {
        MHD_add_response_header(response, "Access-Control-Allow-Origin", restAllowedOrigin);
      }
    }

    if (ciP->verb == OPTIONS)
    {
      MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token");

      char corsMaxAge[STRING_SIZE_FOR_INT];
      snprintf(corsMaxAge, sizeof(corsMaxAge), "%d", restCORSMaxAge);

      MHD_add_response_header(response, "Access-Control-Max-Age", corsMaxAge);
    }
  }

  MHD_queue_response(ciP->connection, ciP->httpStatusCode, response);
  MHD_destroy_response(response);
}



/* ****************************************************************************
*
* tagGet - return a tag (request type) depending on the incoming request string
*
* This function is called only from restErrorReplyGet, but as the parameter
* 'request' is simply 'forwarded' from restErrorReplyGet, the 'request' can
* have various contents - for that the different strings of 'request'.
*/
static std::string tagGet(const std::string& request)
{
  if ((request == "registerContext") || (request == "/ngsi9/registerContext") || (request == "/NGSI9/registerContext") || (request == "registerContextRequest") || (request == "/v1/registry/registerContext"))
    return "registerContextResponse";
  else if ((request == "discoverContextAvailability") || (request == "/ngsi9/discoverContextAvailability") || (request == "/NGSI9/discoverContextAvailability") || (request == "discoverContextAvailabilityRequest") || (request == "/v1/registry/discoverContextAvailability"))
    return "discoverContextAvailabilityResponse";
  else if ((request == "subscribeContextAvailability") || (request == "/ngsi9/subscribeContextAvailability") || (request == "/NGSI9/subscribeContextAvailability") || (request == "subscribeContextAvailabilityRequest") || (request == "/v1/registry/subscribeContextAvailability"))
    return "subscribeContextAvailabilityResponse";
  else if ((request == "updateContextAvailabilitySubscription") || (request == "/ngsi9/updateContextAvailabilitySubscription") || (request == "/NGSI9/updateContextAvailabilitySubscription") || (request == "updateContextAvailabilitySubscriptionRequest") || (request == "/v1/registry/updateContextAvailabilitySubscription"))
    return "updateContextAvailabilitySubscriptionResponse";
  else if ((request == "unsubscribeContextAvailability") || (request == "/ngsi9/unsubscribeContextAvailability") || (request == "/NGSI9/unsubscribeContextAvailability") || (request == "unsubscribeContextAvailabilityRequest") || (request == "/v1/registry/unsubscribeContextAvailability"))
    return "unsubscribeContextAvailabilityResponse";
  else if ((request == "notifyContextAvailability") || (request == "/ngsi9/notifyContextAvailability") || (request == "/NGSI9/notifyContextAvailability") || (request == "notifyContextAvailabilityRequest") || (request == "/v1/registry/notifyContextAvailability"))
    return "notifyContextAvailabilityResponse";

  else if ((request == "queryContext") || (request == "/ngsi10/queryContext") || (request == "/NGSI10/queryContext") || (request == "queryContextRequest") || (request == "/v1/queryContext"))
    return "queryContextResponse";
  else if ((request == "subscribeContext") || (request == "/ngsi10/subscribeContext") || (request == "/NGSI10/subscribeContext") || (request == "subscribeContextRequest") || (request == "/v1/subscribeContext"))
    return "subscribeContextResponse";
  else if ((request == "updateContextSubscription") || (request == "/ngsi10/updateContextSubscription") || (request == "/NGSI10/updateContextSubscription") || (request == "updateContextSubscriptionRequest") || (request == "/v1/updateContextSubscription"))
    return "updateContextSubscriptionResponse";
  else if ((request == "unsubscribeContext") || (request == "/ngsi10/unsubscribeContext") || (request == "/NGSI10/unsubscribeContext") || (request == "unsubscribeContextRequest") || (request == "/v1/unsubscribeContext"))
    return "unsubscribeContextResponse";
  else if ((request == "updateContext") || (request == "/ngsi10/updateContext") || (request == "/NGSI10/updateContext") || (request == "updateContextRequest") || (request == "/v1/updateContext"))
    return "updateContextResponse";
  else if ((request == "notifyContext") || (request == "/ngsi10/notifyContext") || (request == "/NGSI10/notifyContext") || (request == "notifyContextRequest") || (request == "/v1/notifyContext"))
    return "notifyContextResponse";
  else if (request == "StatusCode")
    return "StatusCode";

  return "UnknownTag";
}



/* ****************************************************************************
*
* restErrorReplyGet -
*
* This function renders an error reply depending on the 'request' type.
* Many responses have different syntax and especially the tag in the reply
* differs (registerContextResponse, discoverContextAvailabilityResponse, etc).
*
* Also, the function is called from more than one place, especially from
* restErrorReply, but also from where the payload type is matched against the request URL.
* Where the payload type is matched against the request URL, the incoming 'request' is a
* request and not a response.
*/
std::string restErrorReplyGet(ConnectionInfo* ciP, const std::string& indent, const std::string& request, HttpStatusCode code, const std::string& details)
{
   std::string   tag = tagGet(request);
   StatusCode    errorCode(code, details, "errorCode");
   std::string   reply;

   ciP->httpStatusCode = SccOk;

   if (tag == "registerContextResponse")
   {
      RegisterContextResponse rcr("000000000000000000000000", errorCode);
      reply =  rcr.render();
   }
   else if (tag == "discoverContextAvailabilityResponse")
   {
      DiscoverContextAvailabilityResponse dcar(errorCode);
      reply =  dcar.render();
   }
   else if (tag == "subscribeContextAvailabilityResponse")
   {
      SubscribeContextAvailabilityResponse scar("000000000000000000000000", errorCode);
      reply =  scar.render();
   }
   else if (tag == "updateContextAvailabilitySubscriptionResponse")
   {
      UpdateContextAvailabilitySubscriptionResponse ucas(errorCode);
      reply =  ucas.render();
   }
   else if (tag == "unsubscribeContextAvailabilityResponse")
   {
      UnsubscribeContextAvailabilityResponse ucar(errorCode);
      reply =  ucar.render();
   }
   else if (tag == "notifyContextAvailabilityResponse")
   {
      NotifyContextAvailabilityResponse ncar(errorCode);
      reply =  ncar.render();
   }

   else if (tag == "queryContextResponse")
   {
      QueryContextResponse qcr(errorCode);
      bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
      reply =  qcr.render(ciP->apiVersion, asJsonObject);
   }
   else if (tag == "subscribeContextResponse")
   {
      SubscribeContextResponse scr(errorCode);
      reply =  scr.render();
   }
   else if (tag == "updateContextSubscriptionResponse")
   {
      UpdateContextSubscriptionResponse ucsr(errorCode);
      reply =  ucsr.render();
   }
   else if (tag == "unsubscribeContextResponse")
   {
      UnsubscribeContextResponse uncr(errorCode);
      reply =  uncr.render();
   }
   else if (tag == "updateContextResponse")
   {
      UpdateContextResponse ucr(errorCode);
      bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);
      reply = ucr.render(ciP->apiVersion, asJsonObject);
   }
   else if (tag == "notifyContextResponse")
   {
      NotifyContextResponse ncr(errorCode);
      reply =  ncr.render();
   }
   else if (tag == "StatusCode")
   {
     StatusCode sc(code, details);
     reply = sc.render(false);
   }
   else
   {
      OrionError oe(errorCode);

      LM_T(LmtRest, ("Unknown tag: '%s', request == '%s'", tag.c_str(), request.c_str()));

      ciP->httpStatusCode = oe.code;
      reply = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
   }

   return reply;
}
